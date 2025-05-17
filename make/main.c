#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdbool.h>

#include "uselibpng.h"
#define FRUSTUM_PLANES 6

static const char key_word_png[] = "png";
static const char key_word_color[] = "color";
static const char key_word_position[] = "position";
static const char key_word_sRGB[] = "sRGB";
static const char key_word_depth[] = "depth";
static const char key_word_hyp[] = "hyp";
static const char key_word_drawPixels[] = "drawPixels";
static const char key_word_drawArraysTriangles[] = "drawArraysTriangles";

typedef struct {
    int count; // number of items in each set/offset
    int length; // number of total items
    float *items;
} list_item;

typedef struct {
    float x;
    float y;
    float z;
    float w;
    float r;
    float g;
    float b;
    float a;
    // float s;
    // float t;
} vector;

// it feels wrong to create global attributes...
bool sRGB = false;
bool depth = false;
bool hyp = false;
float **z_buffer;

int frustum_planes[6][4] = {
    { 1,  0,  0,  1},
    {-1,  0,  0,  1},
    { 0,  1,  0,  1},
    { 0, -1,  0,  1},
    { 0,  0,  1,  1},
    { 0,  0, -1,  1}
};

bool prefix(const char *pre, const char *str) {
    return strncmp(pre, str, strlen(pre)) == 0;
}

void add_token(char ***dest, int *current_size, const char *new_token) {
    char **temp = realloc(*dest, (*current_size + 1) * sizeof(char *));

    if (temp == NULL) {
        printf("Memory allocation failed!\n");
        exit(1);
    }

    *dest = temp;

    (*dest)[*current_size] = malloc((strlen(new_token) + 1) * sizeof(char));

    if ((*dest)[*current_size] == NULL) {
        printf("Memory allocation for string failed!\n");
        exit(1);
    }

    strcpy((*dest)[*current_size], new_token);
    (*current_size)++;
}

void tokenize_line(char ***tokens, int *token_count, const char *line, const size_t start_pos, const int line_length) {
    int current_pos = start_pos;
    while (current_pos < line_length && line[current_pos] != '\n') {
        int non_space_length = 0;
        while (!isspace(line[current_pos]) && line[current_pos] != '\0') {
            non_space_length++;
            current_pos++;
        }

        if (non_space_length > 0) {
            char token[non_space_length + 1];
            int c = 0;
            while (c < non_space_length) {
                token[c] = line[current_pos - non_space_length + c];
                c++;
            }
            // terminate
            token[c] = '\0';

            add_token(tokens, token_count, token);
        } else {
            current_pos++;
        }
    }
}

void free_tokens(char **arr, const int size) {
    for (int i = 0; i < size; i++) {
        free(arr[i]);
    }
    free(arr);
}

void free_list_item(list_item *list_item) {
    free(list_item->items);
    free(list_item);
}

bool string_is_number(const char *str) {
    for (int i = 0; i < strlen(str); i++) {
        if (!isdigit(str[i])) {
            return false;
        }
    }

    return true;
}

double linear_to_srgb(float linear) {
    if (linear <= 0.0031308) {
        return linear * 12.92;
    }
    return 1.055 * pow(linear, 1.0 / 2.4) - 0.055;
}

float viewport_coordinates(const int length, const float point, const float w) {
    return (point / w + 1) * length / 2;
}

float device_coordinates(const int length, const float point, const float w) {
    return (point / (length / 2) - 1) * w;
}

bool vec_equals(const vector v1, const vector v2) {
    return v1.x == v2.x &&
           v1.y == v2.y &&
           v1.z == v2.z &&
           v1.w == v2.w &&
           v1.r == v2.r &&
           v1.g == v2.g &&
           v1.b == v2.b &&
           v1.a == v2.a;
}

void buffer_pixel(image_t *img, const int x1, const int y1, const float w1, float r1, float g1, float b1, float a1) {
    if (hyp) {
        r1 = r1 / w1;
        g1 = g1 / w1;
        b1 = b1 / w1;
        a1 = a1 / w1;
    }

    if (sRGB) {
        r1 = linear_to_srgb(r1);
        g1 = linear_to_srgb(g1);
        b1 = linear_to_srgb(b1);
        a1 = linear_to_srgb(a1);
    }

    pixel_xy(img, x1, y1).red = r1 * 255;
    pixel_xy(img, x1, y1).green = g1 * 255;
    pixel_xy(img, x1, y1).blue = b1 * 255;
    pixel_xy(img, x1, y1).alpha = a1 * 255;
}

void scanline(image_t *img, const vector v1, const vector v2) {
    if (vec_equals(v1, v2)) {
        return;
    }

    float x1 = v1.x;
    float y1 = v1.y;
    float z1 = v1.z;
    float w1 = v1.w;
    float r1 = v1.r;
    float g1 = v1.g;
    float b1 = v1.b;
    float a1 = v1.a;
    float x2 = v2.x;
    float z2 = v2.z;
    float w2 = v2.w;
    float r2 = v2.r;
    float g2 = v2.g;
    float b2 = v2.b;
    float a2 = v2.a;

    if (x1 > x2) {
        x2 = v1.x;
        z2 = v1.z;
        w2 = v1.w;
        r2 = v1.r;
        g2 = v1.g;
        b2 = v1.b;
        a2 = v1.a;
        x1 = v2.x;
        y1 = v2.y;
        z1 = v2.z;
        w1 = v2.w;
        r1 = v2.r;
        g1 = v2.g;
        b1 = v2.b;
        a1 = v2.a;
    }

    const float dx = x2 - x1;
    const float dz = z2 - z1;
    const float dw = w2 - w1;
    const float dr = r2 - r1;
    const float dg = g2 - g1;
    const float db = b2 - b1;
    const float da = a2 - a1;

    const float z_s = dz / dx;
    const float w_s = dw / dx;
    const float r_s = dr / dx;
    const float g_s = dg / dx;
    const float b_s = db / dx;
    const float a_s = da / dx;

    const float e = ceil(x1) - x1;

    const float offset_x = e * 1;
    const float offset_z = e * z_s;
    const float offset_w = e * w_s;
    const float offset_r = e * r_s;
    const float offset_g = e * g_s;
    const float offset_b = e * b_s;
    const float offset_a = e * a_s;

    x1 += offset_x;
    z1 += offset_z;
    w1 += offset_w;
    r1 += offset_r;
    g1 += offset_g;
    b1 += offset_b;
    a1 += offset_a;

    while (x1 < x2) {
        if (x1 >= 0 && y1 >= 0 && x1 < img->width && y1 < img->height) {
            if (depth) {
                const int x_cord = (int) x1;
                const int y_cord = (int) y1;
                if (z1 < z_buffer[x_cord][y_cord]) {
                    z_buffer[x_cord][y_cord] = z1;
                    buffer_pixel(img, x1, y1, w1, r1, g1, b1, a1);
                }
            } else {
                buffer_pixel(img, x1, y1, w1, r1, g1, b1, a1);
            }
        }
        z1 += z_s;
        w1 += w_s;
        r1 += r_s;
        g1 += g_s;
        b1 += b_s;
        a1 += a_s;
        x1++;
    }
}

void s_vector(image_t *img, vector *vectors, const vector v1, const vector v2) {
    float x1 = v1.x;
    float y1 = v1.y;
    float z1 = v1.z;
    float w1 = v1.w;
    float r1 = v1.r;
    float g1 = v1.g;
    float b1 = v1.b;
    float a1 = v1.a;
    float x2 = v2.x;
    float y2 = v2.y;
    float z2 = v2.z;
    float w2 = v2.w;
    float r2 = v2.r;
    float g2 = v2.g;
    float b2 = v2.b;
    float a2 = v2.a;

    if (y1 > y2) {
        x2 = v1.x;
        y2 = v1.y;
        z2 = v1.z;
        w2 = v1.w;
        r2 = v1.r;
        g2 = v1.g;
        b2 = v1.b;
        a2 = v1.a;
        x1 = v2.x;
        y1 = v2.y;
        z1 = v2.z;
        w1 = v2.w;
        r1 = v2.r;
        g1 = v2.g;
        b1 = v2.b;
        a1 = v2.a;
    }

    if (hyp) {
        r1 = r1 * w1;
        g1 = g1 * w1;
        b1 = b1 * w1;
        a1 = a1 * w1;
        r2 = r2 * w2;
        g2 = g2 * w2;
        b2 = b2 * w2;
        a2 = a2 * w2;
    }


    //TODO: handle / by 0

    const float dx = x2 - x1;
    const float dy = y2 - y1;
    const float dz = z2 - z1;
    const float dw = w2 - w1;
    const float dr = r2 - r1;
    const float dg = g2 - g1;
    const float db = b2 - b1;
    const float da = a2 - a1;

    const float x_s = dx / dy;
    const float z_s = dz / dy;
    const float w_s = dw / dy;
    const float r_s = dr / dy;
    const float g_s = dg / dy;
    const float b_s = db / dy;
    const float a_s = da / dy;

    const float e = ceil(y1) - y1;

    const float offset_x = e * x_s;
    const float offset_y = e * 1;
    const float offset_z = e * z_s;
    const float offset_w = e * w_s;
    const float offset_r = e * r_s;
    const float offset_g = e * g_s;
    const float offset_b = e * b_s;
    const float offset_a = e * a_s;

    // vector s_vec;
    vectors[0].x = x_s;
    vectors[0].y = 1;
    vectors[0].z = z_s;
    vectors[0].w = w_s;
    vectors[0].r = r_s;
    vectors[0].g = g_s;
    vectors[0].b = b_s;
    vectors[0].a = a_s;

    // vector offset_vec;
    vectors[1].x = x1 + offset_x;
    vectors[1].y = y1 + offset_y;
    vectors[1].z = z1 + offset_z;
    vectors[1].w = w1 + offset_w;
    vectors[1].r = r1 + offset_r;
    vectors[1].g = g1 + offset_g;
    vectors[1].b = b1 + offset_b;
    vectors[1].a = a1 + offset_a;
}

void dda(image_t *img, vector top, vector middle, vector bottom) {
    vector p_long_vectors[2];
    s_vector(img, p_long_vectors, top, bottom);

    vector p_middle_vectors[2];
    s_vector(img, p_middle_vectors, top, middle);

    vector s_long = p_long_vectors[0];
    vector p_long = p_long_vectors[1];
    vector s_middle = p_middle_vectors[0];
    vector p_middle = p_middle_vectors[1];
    while (p_long.y < middle.y) {
        scanline(img, p_long, p_middle);
        p_long.y++;
        p_long.x += s_long.x;
        p_long.z += s_long.z;
        p_long.w += s_long.w;
        p_long.r += s_long.r;
        p_long.g += s_long.g;
        p_long.b += s_long.b;
        p_long.a += s_long.a;

        p_middle.y++;
        p_middle.x += s_middle.x;
        p_middle.z += s_middle.z;
        p_middle.w += s_middle.w;
        p_middle.r += s_middle.r;
        p_middle.g += s_middle.g;
        p_middle.b += s_middle.b;
        p_middle.a += s_middle.a;
    }

    vector p_middle_bottom_vectors[2];
    s_vector(img, p_middle_bottom_vectors, middle, bottom);

    vector s_middle_bottom = p_middle_bottom_vectors[0];
    vector p_middle_bottom = p_middle_bottom_vectors[1];
    while (p_middle_bottom.y < bottom.y) {
        scanline(img, p_long, p_middle_bottom);

        p_long.y++;
        p_long.x += s_long.x;
        p_long.z += s_long.z;
        p_long.w += s_long.w;
        p_long.r += s_long.r;
        p_long.g += s_long.g;
        p_long.b += s_long.b;
        p_long.a += s_long.a;

        p_middle_bottom.y++;
        p_middle_bottom.x += s_middle_bottom.x;
        p_middle_bottom.z += s_middle_bottom.z;
        p_middle_bottom.w += s_middle_bottom.w;
        p_middle_bottom.r += s_middle_bottom.r;
        p_middle_bottom.g += s_middle_bottom.g;
        p_middle_bottom.b += s_middle_bottom.b;
        p_middle_bottom.a += s_middle_bottom.a;
    }
}

void drawArrayTriangles(image_t *img, int first, int count, list_item *positions, list_item *colors) {
    int current_pos = 0;
    while (current_pos < count) {
        vector vectors[3];
        for (int k = 0; k < 3; k++) {
            int start = current_pos + first;

            float z = 0.0f;
            if (positions->count >= 3) {
                z = positions->items[start * positions->count + 2];
            }

            float w = 1.0f;
            if (positions->count >= 4) {
                w = positions->items[start * positions->count + 3];
            }

            vectors[k].x = viewport_coordinates(img->width, positions->items[start * positions->count], w);
            vectors[k].y = viewport_coordinates(img->height, positions->items[start * positions->count + 1], w);

            float a = 1.0f;
            if (colors->count > 3) {
                a = colors->items[start * positions->count * colors->count + 3];
            }

            vectors[k].z = z / w;
            vectors[k].w = 1 / w;
            vectors[k].r = colors->items[start * colors->count];
            vectors[k].g = colors->items[start * colors->count + 1];
            vectors[k].b = colors->items[start * colors->count + 2];
            vectors[k].a = a;

            current_pos++;
        }
        for (int m = 0; m < 3; m++) {
            for (int n = 0; n < 2; n++) {
                if (vectors[n].y > vectors[n + 1].y) {
                    const vector temp = vectors[n];
                    vectors[n] = vectors[n + 1];
                    vectors[n + 1] = temp;
                }
            }
        }
        dda(img, vectors[0], vectors[1], vectors[2]);
    }
}

void free_z_buffer(int width) {
    for (int i = 0; i < width; i++) {
        free(z_buffer[i]);
    }

    free(z_buffer);
}

int main(int argc, const char **argv) {
    if (argc != 2) {
        perror("Usage: main file=...\n");
        return 1;
    }
    int file_name_index = argc - 1;
    const char *file_name = argv[file_name_index];
    printf("file name: %s\n", file_name);

    FILE *file = fopen(file_name, "r");
    if (file == NULL) {
        perror("Error opening file");
        return 1;
    }

    char *line = NULL;
    size_t line_length = 0;
    char *filename = NULL;

    list_item *colors = malloc(2048 * sizeof(float) + sizeof(float));
    list_item *positions = malloc(2048 * sizeof(float) + sizeof(float));
    int *elements = malloc(2048 * sizeof(int) + sizeof(int));

    image_t *img;

    while (getline(&line, &line_length, file) != -1) {
        if (prefix(key_word_png, line)) {
            // may be an edge case where every character ends up in its own column in which this doesn't compensate
            // for the extra null terminator
            char **tokens = malloc(1 * sizeof(char *));
            int token_count = 0;
            tokenize_line(&tokens, &token_count, line, strlen(key_word_png), line_length);

            int dimensions[2];

            // it's hacky, don't judge me
            int num_count = 0;

            for (int i = 0; i < token_count; i++) {
                if (string_is_number(tokens[i])) {
                    dimensions[num_count] = atoi(tokens[i]);
                    num_count++;
                }
                filename = malloc(strlen(tokens[i]) + 1);
                strcpy(filename, tokens[i]);
            }
            img = new_image(dimensions[0], dimensions[1]);

            free_tokens(tokens, token_count);
        } else if (prefix(key_word_color, line)) {
            char **tokens = malloc(1 * sizeof(char *));
            int token_count = 0;
            tokenize_line(&tokens, &token_count, line, strlen(key_word_color), line_length);

            colors->items = malloc(token_count * sizeof(float));
            colors->length = token_count - 1;
            colors->count = atoi(tokens[0]);
            for (int i = 1; i < token_count; i++) {
                char *pEnd;
                colors->items[i - 1] = strtof(tokens[i], &pEnd);
            }

            free_tokens(tokens, token_count);
        } else if (prefix(key_word_position, line)) {
            char **tokens = malloc(1 * sizeof(char *));
            int token_count = 0;
            tokenize_line(&tokens, &token_count, line, strlen(key_word_position), line_length);

            positions->items = malloc(token_count * sizeof(float));
            positions->length = token_count - 1;
            positions->count = atoi(tokens[0]);
            for (int i = 1; i < token_count; i++) {
                char *pEnd;
                positions->items[i - 1] = strtof(tokens[i], &pEnd);
            }

            free_tokens(tokens, token_count);
        } else if (prefix(key_word_drawPixels, line)) {
            char **tokens = malloc(1 * sizeof(char *));
            int token_count = 0;
            tokenize_line(&tokens, &token_count, line, strlen(key_word_drawPixels), line_length);

            // n = atoi(tokens[0]);

            free_tokens(tokens, token_count);
        } else if (prefix(key_word_drawArraysTriangles, line)) {
            char **tokens = malloc(1 * sizeof(char *));
            int token_count = 0;
            tokenize_line(&tokens, &token_count, line, strlen(key_word_drawArraysTriangles), line_length);

            drawArrayTriangles(img, atoi(tokens[0]), atoi(tokens[1]), positions, colors);

            free_tokens(tokens, token_count);
        } else if (prefix(key_word_sRGB, line)) {
            sRGB = true;
        } else if (prefix(key_word_depth, line)) {
            depth = true;
            z_buffer = malloc(sizeof(float *) * img->width);
            for (int i = 0; i < img->width; i++) {
                z_buffer[i] = malloc(sizeof(float) * img->height);
                for (int j = 0; j < img->height; j++) {
                    z_buffer[i][j] = INFINITY;
                }
            }
        } else if (prefix(key_word_hyp, line)) {
            hyp = true;
            sRGB = true;
            depth = true;
        }
    }

    fclose(file);

    save_image(img, filename);

    free(filename);
    free_list_item(colors);
    free_list_item(positions);
    if (depth) {
        free_z_buffer(img->width);
    }
    free_image(img);

    return 0;
}
