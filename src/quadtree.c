#include "quadtree.h"
#include <stdlib.h>
#include <string.h>

static QuadTreeNode* create_node(Boundary boundary) {
    QuadTreeNode* node = (QuadTreeNode*)malloc(sizeof(QuadTreeNode));
    node->boundary = boundary;
    node->particles = (vec2*)malloc(MAX_PARTICLES_PER_NODE * sizeof(vec2));
    node->numParticles = 0;
    node->isDivided = false;
    node->northwest = NULL;
    node->northeast = NULL;
    node->southwest = NULL;
    node->southeast = NULL;
    node->totalMass = 0.0f;
    node->centerOfMass[0] = 0.0f;
    node->centerOfMass[1] = 0.0f;
    return node;
}

static void subdivide(QuadTreeNode* node) {
    float x = node->boundary.x;
    float y = node->boundary.y;
    float w = node->boundary.width * 0.5f;
    float h = node->boundary.height * 0.5f;

    Boundary nw = {x, y, w, h};
    Boundary ne = {x + w, y, w, h};
    Boundary sw = {x, y + h, w, h};
    Boundary se = {x + w, y + h, w, h};

    node->northwest = create_node(nw);
    node->northeast = create_node(ne);
    node->southwest = create_node(sw);
    node->southeast = create_node(se);
    node->isDivided = true;
}

static bool is_in_boundary(Boundary* b, vec2 point) {
    return point[0] >= b->x && point[0] < b->x + b->width &&
           point[1] >= b->y && point[1] < b->y + b->height;
}

static void insert_particle(QuadTreeNode* node, vec2 position) {
    if (!is_in_boundary(&node->boundary, position)) {
        return;
    }

    if (node->numParticles < MAX_PARTICLES_PER_NODE && !node->isDivided) {
        memcpy(node->particles[node->numParticles], position, sizeof(vec2));
        node->numParticles++;
        
        // Update center of mass
        node->centerOfMass[0] = (node->centerOfMass[0] * node->totalMass + position[0]) / (node->totalMass + 1);
        node->centerOfMass[1] = (node->centerOfMass[1] * node->totalMass + position[1]) / (node->totalMass + 1);
        node->totalMass += 1.0f;
        return;
    }

    if (!node->isDivided) {
        subdivide(node);
        // Redistribute existing particles
        for (int i = 0; i < node->numParticles; i++) {
            insert_particle(node->northwest, node->particles[i]);
            insert_particle(node->northeast, node->particles[i]);
            insert_particle(node->southwest, node->particles[i]);
            insert_particle(node->southeast, node->particles[i]);
        }
    }

    insert_particle(node->northwest, position);
    insert_particle(node->northeast, position);
    insert_particle(node->southwest, position);
    insert_particle(node->southeast, position);
}

static void cleanup_node(QuadTreeNode* node) {
    if (node->isDivided) {
        cleanup_node(node->northwest);
        cleanup_node(node->northeast);
        cleanup_node(node->southwest);
        cleanup_node(node->southeast);
        free(node->northwest);
        free(node->northeast);
        free(node->southwest);
        free(node->southeast);
    }
    free(node->particles);
}

void quadtree_init(QuadTree* qt, vec2* positions, int numParticles, float worldSize) {
    Boundary boundary = {-worldSize/2, -worldSize/2, worldSize, worldSize};
    qt->root = create_node(boundary);
    qt->particlePositions = positions;
    qt->totalParticles = numParticles;
}

void quadtree_cleanup(QuadTree* qt) {
    cleanup_node(qt->root);
    free(qt->root);
}

void quadtree_update(QuadTree* qt) {
    // Clear and rebuild the tree
    cleanup_node(qt->root);
    Boundary boundary = qt->root->boundary;
    free(qt->root);
    qt->root = create_node(boundary);

    // Reinsert all particles
    for (int i = 0; i < qt->totalParticles; i++) {
        insert_particle(qt->root, qt->particlePositions[i]);
    }
} 