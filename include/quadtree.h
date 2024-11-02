#ifndef QUADTREE_H
#define QUADTREE_H

#include <stdbool.h>
#include "cglm/types.h"

#define MAX_PARTICLES_PER_NODE 4

typedef struct {
    float x;
    float y;
    float width;
    float height;
} Boundary;

typedef struct QuadTreeNode {
    Boundary boundary;
    vec2* particles;        // Array of particle positions in this node
    int numParticles;       // Number of particles in this node
    bool isDivided;         // Whether this node has children
    struct QuadTreeNode* northwest;
    struct QuadTreeNode* northeast;
    struct QuadTreeNode* southwest;
    struct QuadTreeNode* southeast;
    vec2 centerOfMass;     // Center of mass for this node
    float totalMass;       // Total mass of particles in this node
} QuadTreeNode;

typedef struct {
    QuadTreeNode* root;
    vec2* particlePositions;  // Reference to particle positions
    int totalParticles;
} QuadTree;

void quadtree_init(QuadTree* qt, vec2* positions, int numParticles, float worldSize);
void quadtree_cleanup(QuadTree* qt);
void quadtree_update(QuadTree* qt);

#endif 