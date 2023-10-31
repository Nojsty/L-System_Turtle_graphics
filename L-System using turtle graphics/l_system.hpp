#pragma once

#include "draw_primitives.hpp"
#include "glm_headers.hpp"
#include <vector>
#include <unordered_map>
#include <string>
#include <stack>

/// Representation of the turtle (from the turtle geometry)
struct TurtleBase {

    virtual ~TurtleBase() {}

    glm::vec3 const& position() const {
        
        return currentState.position;
    }
    
    glm::vec3 const& forward() const {
        
        return currentState.forward;
    }

    glm::vec3 const& left() const {
        
        return currentState.left;
    }
    
    glm::vec3 const& up() const {
        
        return currentState.up;
    }

    float brush_width() const {
        
        return currentState.brushWidth;
    }
    
    void move(float const distance) {
        
        currentState.position += ( distance * glm::normalize( currentState.forward ) );
    }

    void rotate(glm::vec3 const& unit_axis, float const angle_radians) {
        
        // create a transformation matrix
        glm::mat3 rotationMatrix = glm::toMat3( glm::angleAxis( angle_radians, unit_axis ) );

        // transform forward
        glm::vec3 newForward = rotationMatrix * currentState.forward;

        // transform left
        glm::vec3 newLeft = rotationMatrix * currentState.left;

        // => cross product (forward, left) => new up
        glm::vec3 newUp = glm::cross( newForward, newLeft );

        // => cross product (up, forward) => new left
        newLeft = glm::cross( newUp, newForward );

        // normalize all 3 vectors and assign
        currentState.forward = glm::normalize(newForward);
        currentState.left = glm::normalize(newLeft);
        currentState.up = glm::normalize(newUp);
    }

    void set_brush_width(float const width) {
        
        if ( width > 0.0 ) 
            currentState.brushWidth = width;
    }


    void push() {
        
        turtleStack.push( currentState );
    }

    void pop() {
        
        if ( !turtleStack.empty() )
        {
            // save the last state
            TurtleState lastState = turtleStack.top();
            turtleStack.pop();

            // update current state to last state
            currentState.position = lastState.position;
            currentState.forward = lastState.forward;
            currentState.left = lastState.left;
            currentState.up = lastState.up;
            currentState.brushWidth = lastState.brushWidth;
        }
    }

private:

	// initial values for the state of the turtle
    struct TurtleState {
        glm::vec3 position = glm::vec3(0.0, 0.0, 0.0);
        glm::vec3 forward = glm::vec3(0.0, 1.0, 0.0);
        glm::vec3 left = glm::vec3(0.0, 0.0, 1.0);
        glm::vec3 up = glm::vec3(1.0, 0.0, 0.0);

        float brushWidth = 1.0f;
    };

    // initialize origin state 
    TurtleState currentState;
    
    // stack for saving states of the turtle
    std::stack<TurtleState> turtleStack;
};

/// A specialisation of the general turtle type "TurtleBase"
/// commanding the turtle according to passed L-system rules.
struct LTurtle : public TurtleBase {

    /// Stores several constants (numbers) representing
    /// the configuration of the LTrutle.
    struct Config {
        float radius;                   
        float distance;                 
        float leaf_size;                
        float angle_world_y;            
        float angle_turtle_left;        
        float brush_decay_coef;         
                                        
        unsigned int max_depth;         
    };

    /// A type for rules of a L-system.
    using Rules = std::unordered_map<char, std::string>;

    /// Construct a turtle for a passed config and L-system rules.
    LTurtle(
        Config const& cfg_,                     
        Rules const& rules_,                    
        std::vector<Branch>& branches_ref,      
        std::vector<Leaf>& leaves_ref           
        )
        : TurtleBase()
        , cfg(cfg_)
        , rules(rules_)
        , branches(branches_ref)
        , leaves(leaves_ref)
    {}

    /// Getter of the config data.
    Config const& config() const { return cfg; }

    /// Applies L-system's rules to the passed sentence (axiom) and
    /// generates and saves corresponding geometrical objects by
    /// calling the "process" method.
    void run(std::string const& sentence, unsigned int depth = 0U) {
        
        if ( depth < config().max_depth )
        {
            // apply rules according to unordered_map Rules
            for ( char c : sentence )
            {
                std::unordered_map<char, std::string>::const_iterator foundRule = rules.find(c);

                // if character not found -> process() will discard it
                if ( foundRule == rules.end() )
                {
                    process( c );
                }
                else
                {
					// recursively lower higher depth
                    run( foundRule->second, depth + 1 );   
                }
            }
        }
        else
        {
            for ( char c : sentence )
            {
                process( c );
            }
        }
    }

    /// Commands the turtle based on the passed symbol.
    void process(char const symbol) {
        switch (symbol)
        {
        case 'L':
			// create an instance of Leaf and store it in leaves
            leaves.push_back( Leaf( position(), forward(), left(), 
                                 glm::vec2( config().leaf_size * brush_width(), config().leaf_size * brush_width() * 2 ) )
                            );

			// move turtle forward
            move( config().distance * brush_width() );

            break;
        case 'l':
			// create an instance of Leaf and store it in leaves
            leaves.push_back( Leaf( position(), forward(), left(), 
                                 glm::vec2( config().leaf_size * brush_width(), config().leaf_size * brush_width() * 2 ) )
                            );

			// move turtle forward
            move( config().distance * brush_width() );

            break;
        case 'B':
            // create an instance of Branch and store it in branches
            branches.push_back( Branch( position(), config().radius * brush_width(), 
                                       position() + ( config().distance * brush_width() * forward() ), config().brush_decay_coef * config().radius * brush_width() )
                              );

			// move turtle forward
            move( config().distance * brush_width() );

            break;
        case 'M':
			// move turtle forward
            move( config().distance * brush_width() );

            break;
        case '+':
			// rotate the turtle about Y-axis in positive direction
            rotate( glm::vec3(0.0f, 1.0f, 0.0f), config().angle_world_y );

            break;
        case '-':
			// rotate the turtle about Y-axis in negative direction
            rotate( glm::vec3(0.0f, 1.0f, 0.0f), -config().angle_world_y );

            break;
        case '&':
			// rotate the turtle about turtle's left vector in positive direction
            rotate( glm::normalize( left() ), config().angle_turtle_left );

            break;
        case '^':
            // rotate the turtle about turtle's left vector in negative direction
            rotate( glm::normalize( left() ), -config().angle_turtle_left );

            break;
        case '*':
            /// set turtle brush width
            set_brush_width ( config().brush_decay_coef * brush_width() );

            break;
        case '[':
			// store current turtle state
            push();

            break;
        case ']':
			// retrieve last turtle state
            pop();

            break;
        default:
            break;
        }
    }

private:
    Config cfg;                         
    Rules rules;                        
    std::vector<Branch>& branches;      
    std::vector<Leaf>& leaves;         
};
