#version 450 core

// The definition of branch as rounded cone.
struct Branch
{
    vec3 p1;  
    float r1; 
    vec3 p2;  
    float r2; 
};

// The definition of leaf.
struct Leaf
{
    vec4 position;   
    vec4 direction; 
    vec4 up;		
    vec4 size;		
};

layout (binding = 0) uniform samplerCube skybox_tex; 
layout (binding = 1) uniform sampler2D wood_tex; 
layout (binding = 2) uniform sampler2D laef_tex; 

// The definition of a ray.
struct Ray {
    vec3 origin;     
    vec3 direction;  
};
// The definition of an intersection.
struct Hit {
    vec3 intersection;        
	float t;				   
    vec3 normal;              
	vec3 material;			  
};
const Hit miss = Hit(vec3(0.0), 1e20, vec3(0.0), vec3(0.f));

// Computes the branch color based on UV coordinates.
vec3 getBranchMaterial(vec3 intersection, Branch branch){
	
	// compute vectors w and q
	vec3 w = branch.p2 - branch.p1;
	w = normalize( w );
	vec3 q = intersection - branch.p1;

	// V for texture
	float v = dot( w, q );

	// vectors t and s
	vec3 t = cross( w, vec3( 0, 0, 1 ) );
	vec3 s = cross( w, t );

	// recalculate with new set vector for t
	if ( t == vec3( 0, 0, 0 ) || dot( q, s ) == 0 )
	{
		t = cross( w, vec3( 0, 1, 0 ) );
		s = cross( w, t );
	}

	t = normalize( t );
	s = normalize( s );

	// U for texture
	float u = atan( ( dot( q, t ) ) / ( dot( q, s ) ) );

    vec3 tex_crds = vec3( u * 0.2, v * 0.5, 0 );				// scale UVs with (0.2, 0.5)
	
	// color from interesection from texture 'wood_tex'
	vec4 texColor = texture( wood_tex, vec2(u, v) );			
	
	return texColor.xyz;
}

// Computes an intersection between a ray and a leaf (defined as parallelogram).
Hit RayLeafIntersection(Ray ray, Leaf leaf){

	vec4 color = texture( laef_tex, vec2( u, v ) );
	
	// if leaf hit
	if ( color.w > 0.1 )
	{
		// check if normal needs to be flipped
		if ( dot( ray.direction, n ) > 0 ) 
		{
			// flip normal
			n = -n;
		}

		return Hit( intersection, t, normalize( n ), color.xyz );
	}
	else
	{
		return miss;
	}
}

// Evaluates the intersections of the ray with the scene objects and returns the closes hit.
Hit Evaluate(Ray ray){
	// Sets the closes hit either to miss or to an intersection with the plane representing the ground.
	Hit closest_hit = RayPlaneIntersection(ray, vec3(0, 1, 0), vec3(0));

	for(int i = 0; i < num_branches; i++){
		Hit intersection = RayBranchIntersection(ray, branches[i]);
		if(intersection.t < closest_hit.t){
			closest_hit = intersection;
		}
	}

	for(int i = 0; i < num_leaves; i++){
		Hit intersection = RayLeafIntersection(ray, leaves[i]);
		if(intersection.t < closest_hit.t){
			closest_hit = intersection;
		}
	}

    return closest_hit;
}

// Traces the ray trough the scene and accumulates the color.
vec3 Trace(Ray ray) {
	const float epsilon = 0.01;

	Hit hit = Evaluate(ray);
	if (hit != miss)
	{
		// add diffuse lighting
		vec3 A = 0.2 * hit.material;
		vec3 D = 0.8 * hit.material * lights[0].diffuse * max( dot( hit.normal, L ), 0 );
		color = A + D;

		// ray for shadow
		Ray shadowRay;
		shadowRay.origin = hit.intersection + vec3(epsilon, epsilon, epsilon);
		shadowRay.direction = L;

		// if ray for shadow hits something -> cast shadow
		Hit shadowHit = Evaluate( shadowRay );
		if ( shadowHit != miss )
		{
			// cast shadow
			color = 0.2 * color;
		}
	} 
	// everything missed -> sample skybox
	else
	{
		vec4 texColor = texture( skybox_tex, ray.direction );
		color = texColor.xyz;
	}

    return color;
}