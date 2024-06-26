#ifndef GRAPH_H
#define GRAPH_H
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <errno.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>      // For open(), O_RDONLY, etc.
#include <sys/mman.h>   // For mmap(), PROT_READ, MAP_SHARED, MAP_FAILED, etc.

typedef struct graph {
  // TODO: Fix spelling
  uint64_t n_verticies;
  uint64_t n_edges;
  uint64_t *verticies;
  uint64_t *neighbors;
  uint64_t *distances;
} graph_t;

// Interface
static uint64_t get_neighbors(graph_t *g, uint64_t index, uint64_t **neighbors);
static graph_t *parse_mtx_file(char *fp, bool directed);

static char* create_binary_filename(char *filename) {
    const char *suffix = ".bin";

    // Allocate memory for the new string
    // Length of original filename + length of suffix + 1 for null terminator
    size_t len = strlen(filename) + strlen(suffix) + 1;
    char *binary_filename = malloc(len);

    if (binary_filename == NULL) {
        perror("Failed to allocate memory for binary filename");
        exit(EXIT_FAILURE);
    }

    // Copy the original filename into the new string
    strcpy(binary_filename, filename);

    // Append the suffix
    strcat(binary_filename, suffix);

    return binary_filename;
}

// Helper function to check if a file exists
static bool file_exists(char *filename) {
    struct stat buffer;
    return (stat(filename, &buffer) == 0);
}

// Load the binary graph from memory
static graph_t* mmap_graph(char *filename) {
    int fd = open(filename, O_RDONLY);
    if (fd == -1) {
        perror("Failed to open file for reading");
        exit(EXIT_FAILURE);
    }

    struct stat sb;
    if (fstat(fd, &sb) == -1) {
        perror("Failed to stat file");
        exit(EXIT_FAILURE);
    }

    void *file_memory = mmap(NULL, sb.st_size, PROT_READ, MAP_SHARED, fd, 0);
    if (file_memory == MAP_FAILED) {
        perror("Failed to mmap file");
        exit(EXIT_FAILURE);
    }

    graph_t *g = malloc(sizeof(graph_t));
    char *ptr = (char *)file_memory;

    // Pointer arithmetic to read out the different fields
    g->n_verticies = *(uint64_t *)ptr;
    ptr += sizeof(uint64_t);
    g->n_edges = *(uint64_t *)ptr;
    ptr += sizeof(uint64_t);

    g->verticies = (uint64_t *)ptr;
    ptr += (g->n_verticies+1) * sizeof(uint64_t);

    g->neighbors = (uint64_t *)ptr;

    g->distances = (uint64_t*) malloc(sizeof(uint64_t) * (g->n_verticies + 1));
    for (uint64_t i=1; i<=g->n_verticies; i++) {
        g->distances[i] = UINT64_MAX;
    }

    close(fd);
    return g;
}

// Serialize the parsed graph to memory
static void serialize_graph(const char *filename, graph_t *g) {
    FILE *file = fopen(filename, "wb");
    if (!file) {
        perror("Failed to open file for writing");
        exit(EXIT_FAILURE);
    }

    fwrite(&g->n_verticies, sizeof(uint64_t), 1, file);
    fwrite(&g->n_edges, sizeof(uint64_t), 1, file);
    fwrite(g->verticies, sizeof(uint64_t), g->n_verticies + 1, file);
    fwrite(g->neighbors, sizeof(uint64_t), g->n_edges, file);

    fclose(file);
}

/*Idea based on https://www.usenix.org/system/files/login/articles/login_winter20_16_kelly.pdf*/
static graph_t* parse_mtx_file(char* fp, bool directed) {
    // First try to see if it has already been parsed
    char *bin_fp = create_binary_filename(fp);
    if (file_exists(bin_fp)) {
        graph_t* g = mmap_graph(bin_fp);
        free(bin_fp);
        return g;
    }

    // Just parse it as normally
    FILE *mtx_file = fopen(fp, "r");

    if (mtx_file == NULL){
        fprintf(stderr, "Could not find mtx file: %s \n", fp);
    }
    char *line = NULL;
    size_t len = 0;
    uint8_t first = true;

    uint64_t *verticies;
    uint64_t *offsets;
    uint64_t *neighbors;
    uint64_t n_verticies;
    uint64_t n_edges;

    while (getline(&line, &len, mtx_file) != -1) {
        if (line[0] == '%') continue;
        if (first) {
            sscanf(line, "%lu %lu %lu", &n_verticies, &n_verticies, &n_edges);
            first = false;
            assert(n_verticies > 0);
            if (!directed) n_edges *= 2;
            verticies = (uint64_t*) calloc(n_verticies + 1, sizeof(uint64_t));
            neighbors = (uint64_t*) malloc(sizeof(uint64_t) * n_edges);

            if (!verticies || !neighbors) {
                printf("Error in allocating graph\n");
                exit(EXIT_FAILURE);
            }
        }
        else {
            uint64_t vertex;
            uint64_t neighbor;
            //unused
            uint64_t weight;

            sscanf(line, "%lu %lu", &vertex, &neighbor);
            //Starts with 1
            verticies[vertex]++;

            if (!directed) {
                verticies[neighbor]++;
            }
        }
    }
    //Set the indexes
    uint64_t sum = 0;

    for (uint64_t i = 1; i <= n_verticies; i++){
        uint64_t count = verticies[i];
        if (count > 0) {
            verticies[i] += sum;
            sum += count;
        }
        else {verticies[i] = sum;}
        if (i > 1) {
        assert(verticies[i - 1] <= verticies[i]);

        }
    }
    rewind(mtx_file);
    first = true;
    while (getline(&line, &len, mtx_file) != -1) {
        if (line[0] == '%') continue;
        if (first) {
            first = false;
            continue;
        }
        else {
            uint64_t vertex;
            uint64_t neighbor;
            //unused
            uint64_t weight;
            sscanf(line, "%lu %lu", &vertex, &neighbor);

            assert(neighbor <= n_verticies);
            assert(vertex <= n_verticies);

            verticies[vertex]--;
            neighbors[verticies[vertex]] = neighbor;

            if (!directed) {
                verticies[neighbor]--;
                neighbors[verticies[neighbor]] = vertex;
            }

        }
    }
    fclose(mtx_file);

    for (uint64_t i = 2; i <= n_verticies; i++){
        assert(verticies[i - 1] <= verticies[i]);
        }

    graph_t* g = (graph_t*) malloc(sizeof(graph_t));
    uint64_t *distances = (uint64_t*) malloc(sizeof(uint64_t) * (n_verticies + 1));

    //fill distances
    for (uint64_t i=1; i<=n_verticies; i++) {
        distances[i] = UINT64_MAX;
    }

    g->n_verticies = n_verticies;
    g->n_edges = n_edges;
    g->verticies = verticies;
    g->neighbors = neighbors;
    g->distances = distances;

    serialize_graph(bin_fp, g);
    free(bin_fp);

    return g;
}

//Maybe optimize by sending n_verticies as an argument
static uint64_t get_neighbors(graph_t *g, uint64_t index, uint64_t **neighbors) {
    uint64_t loc = g->verticies[index];
    *neighbors = &g->neighbors[loc];
    if (index == g->n_verticies){
        return g->n_edges - loc;
    }
    else return g->verticies[index + 1] - loc;
}

#endif
