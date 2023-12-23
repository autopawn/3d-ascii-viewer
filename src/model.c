#include "model.h"

#include "triangularization.h"

#include <assert.h>
#include <libgen.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

static struct model *model_init(void)
{
    struct model *model = malloc(sizeof(*model));

    model->faces_capacity = 1;
    if (!(model->faces = malloc(model->faces_capacity * sizeof(*model->faces))))
    {
        fprintf(stderr, "ERROR: Memory allocation failure.\n");
        exit(1);
    }
    model->faces_count = 0;

    model->vertex_capacity = 1;
    if (!(model->vertexes = malloc(model->vertex_capacity * sizeof(*model->vertexes))))
    {
        fprintf(stderr, "ERROR: Memory allocation failure.\n");
        exit(1);
    }
    model->vertex_count = 0;

    model->materials_capacity = 1;
    if (!(model->materials = malloc(model->materials_capacity * sizeof(*model->materials))))
    {
        fprintf(stderr, "ERROR: Memory allocation failure.\n");
        exit(1);
    }
    model->materials_count = 0;

    return model;
}

static void model_add_vertex(struct model *model, vec3 vec)
{
    if (model->vertex_count == model->vertex_capacity)
    {
        model->vertex_capacity *= 2;
        if (!(model->vertexes = realloc(model->vertexes, model->vertex_capacity * sizeof(*model->vertexes))))
        {
            fprintf(stderr, "ERROR: Memory allocation failure.\n");
            exit(1);
        }
    }

    model->vertexes[model->vertex_count] = vec;
    model->vertex_count++;
}

static int obj_derelativize_idx(int i, int n)
{
    if (i < -n || i == 0)
    {
        fprintf(stderr, "WARN: Invalid vertex index %d.\n", i);
        return 0;
    }

    if (i < 0)
        return n + i;

    return i - 1;
}

static bool model_validate_idxs(struct model *model)
{
    bool valid = true;

    for (int f = 0; f < model->faces_count; ++f)
    {
        for (int i = 0; i < 3; ++i)
        {
            if (model->faces[f].idxs[i] >= model->vertex_count)
            {
                fprintf(stderr, "WARN: Invalid vertex index %d.\n", model->faces[f].idxs[i]);
                valid = false;
                model->faces[f].idxs[i] = 0;
            }
        }
    }
    return valid;
}

static void model_add_face(struct model *model, int idx1, int idx2, int idx3, int material)
{
    if (model->faces_count == model->faces_capacity)
    {
        model->faces_capacity *= 2;
        if (!(model->faces = realloc(model->faces, model->faces_capacity * sizeof(*model->faces))))
        {
            fprintf(stderr, "ERROR: Memory allocation failure.\n");
            exit(1);
        }
    }
    model->faces[model->faces_count].idxs[0] = idx1;
    model->faces[model->faces_count].idxs[1] = idx2;
    model->faces[model->faces_count].idxs[2] = idx3;
    model->faces[model->faces_count].material = material;

    model->faces_count++;
}

static void model_add_material(struct model *model, const char *name, float d_r, float d_g, float d_b)
{
    if (strlen(name) >= MATERIAL_NAME_BUFFER_SIZE)
    {
        fprintf(stderr, "ERROR: Material name too long.\n");
        exit(1);
    }

    if (model->materials_count == model->materials_capacity)
    {
        model->materials_capacity *= 2;
        if (!(model->materials = realloc(model->materials, model->materials_capacity * sizeof(*model->materials))))
        {
            fprintf(stderr, "ERROR: Memory allocation failure.\n");
            exit(1);
        }
    }

    strcpy(model->materials[model->materials_count].name, name);
    model->materials[model->materials_count].Kd_r = d_r;
    model->materials[model->materials_count].Kd_g = d_g;
    model->materials[model->materials_count].Kd_b = d_b;

    model->materials_count++;
}

int model_get_material_idx(struct model *model, const char *name)
{
    for (int i = 0; i < model->materials_count; ++i)
    {
        if (strcmp(model->materials[i].name, name) == 0)
            return i;
    }
    return -1;
}

void model_invert_triangles(struct model *model)
{
    for (int f = 0; f < model->faces_count; ++f)
    {
        int aux = model->faces[f].idxs[1];
        model->faces[f].idxs[1] = model->faces[f].idxs[2];
        model->faces[f].idxs[2] = aux;
    }
}

void model_normalize(struct model *model)
{
    vec3 center = get_bounding_box_center(model->vertexes, model->vertex_count);

    for (int i = 0; i < model->vertex_count; ++i)
    {
        model->vertexes[i].x -= center.x;
        model->vertexes[i].y -= center.y;
        model->vertexes[i].z -= center.z;
    }

    float max_mag = get_max_dist(model->vertexes, model->vertex_count, (vec3){0, 0, 0});

    float scale = (max_mag == 0) ? 1.0 : 1.0 / max_mag;
    for (int i = 0; i < model->vertex_count; ++i)
    {
        model->vertexes[i].x *= scale;
        model->vertexes[i].y *= scale;
        model->vertexes[i].z *= scale;
    }
}

void model_change_orientation(struct model *model, int axis1, int axis2, int axis3)
{
    assert(0 <= axis1 && axis1 <= 2);
    assert(0 <= axis2 && axis2 <= 2);
    assert(0 <= axis3 && axis3 <= 2);

    for (int i = 0; i < model->vertex_count; ++i)
    {
        vec3 vertex;

        vertex.x = model->vertexes[i].x;
        if (axis1 == 1)
            vertex.x = model->vertexes[i].y;
        if (axis1 == 2)
            vertex.x = model->vertexes[i].z;

        vertex.y = model->vertexes[i].x;
        if (axis2 == 1)
            vertex.y = model->vertexes[i].y;
        if (axis2 == 2)
            vertex.y = model->vertexes[i].z;

        vertex.z = model->vertexes[i].x;
        if (axis3 == 1)
            vertex.z = model->vertexes[i].y;
        if (axis3 == 2)
            vertex.z = model->vertexes[i].z;

        model->vertexes[i] = vertex;
    }
}

void model_invert_x(struct model *model)
{
    for (int i = 0; i < model->vertex_count; ++i)
            model->vertexes[i].x *= -1;
    model_invert_triangles(model);
}

void model_invert_y(struct model *model)
{
    for (int i = 0; i < model->vertex_count; ++i)
            model->vertexes[i].y *= -1;
    model_invert_triangles(model);
}

void model_invert_z(struct model *model)
{
    for (int i = 0; i < model->vertex_count; ++i)
            model->vertexes[i].z *= -1;
    model_invert_triangles(model);
}

void model_free(struct model *model)
{
    free(model->vertexes);
    free(model->faces);
    free(model->materials);
    free(model);
}

// Breaks the string the first time the delim substring is encountered,
// returns the first part of the string.
// The *str pointer is updated to the position after the delim, or NULL if the string ends.
static char *str_chop(char **str, char *delim)
{
    int delim_i = 0;
    char *ret = *str;
    char *p = *str;

    assert(*delim != '\0');

    if (p == NULL)
        return NULL;

    while (*p != '\0')
    {
        if (*p == delim[delim_i])
            delim_i++;
        else
            delim_i = 0;

        p++;

        if (delim[delim_i] == '\0')
        {
            p[-delim_i] = '\0';

            *str = p;
            return ret;
        }
    }

    *str = NULL;
    return ret;
}

static char *str_chop_skip_empty(char **str, char *delim)
{
    char *res;

    while ((res = str_chop(str, delim)))
    {
        if (res[0] != '\0')
            return res;
    }
    return NULL;
}

static bool parse_float(char **buffer, float *f)
{
    char *arg = str_chop_skip_empty(buffer, " ");
    if (!arg)
        return false;
    char *f_str = str_chop(&arg, "/");
    *f = (float) atof(f_str);
    return true;
}

static bool parse_int(char **buffer, int *i)
{
    char *arg = str_chop_skip_empty(buffer, " ");
    if (!arg)
        return false;
    char *i_str = str_chop(&arg, "/");
    *i = atoi(i_str);
    return true;
}

// Remove end-of-line characters and turn tabs into spaces
static void string_strip(char *str)
{
    char *p = str;
    while (*p)
    {
        if (*p == '\n' || *p == '\r')
            *p = '\0';
        if (*p == '\t')
            *p = ' ';
        p++;
    }
}

static void model_load_materials_from_mtl(struct model *model, const char *mtl_fname)
{
    FILE *fp = fopen(mtl_fname, "r");
    if (!fp)
    {
        fprintf(stderr, "WARN: failed to load file \"%s\".\n", mtl_fname);
        return;
    }

    // Read each line of the file
    char buffer[256];

    while (fgets(buffer, sizeof(buffer), fp))
    {
        string_strip(buffer);

        char *bufferp = buffer;
        char *instr = str_chop_skip_empty(&bufferp, " ");

        if (!instr || instr[0] == '#')
            continue;

        if (strcmp(instr, "newmtl") == 0)
        {
            const char *name = str_chop_skip_empty(&bufferp, " ");

            model_add_material(model, name, 1.0, 1.0, 1.0);
        }
        else if (strcmp(instr, "Kd") == 0)
        {
            if (model->materials_count == 0)
            {
                fprintf(stderr, "WARN: Expected newmtl before \"%s\" instruction.\n", instr);
                continue;
            }

            float r, g, b;
            if (!parse_float(&bufferp, &r) || !parse_float(&bufferp, &g) || !parse_float(&bufferp, &b))
            {
                fprintf(stderr, "WARN: invalid \"%s\" instruction.\n", instr);
                continue;
            }

            model->materials[model->materials_count - 1].Kd_r = r;
            model->materials[model->materials_count - 1].Kd_g = g;
            model->materials[model->materials_count - 1].Kd_b = b;
        }
    }

    fclose(fp);
}

// Read a line, duplicating the size of the buffer if necessary, using only gets() for portability.
static char *read_line(char **buffer, int *buffer_size, FILE *fp)
{
    if (!fgets(*buffer, *buffer_size, fp))
        return NULL;

    while ((*buffer)[strlen(*buffer) - 1] != '\n')
    {
        if (!(*buffer = realloc(*buffer, *buffer_size * 2)))
        {
            fprintf(stderr, "ERROR: Memory allocation failure.\n");
            exit(1);
        }

        if (!fgets(*buffer + *buffer_size - 1, *buffer_size + 1, fp))
        {
            *buffer_size *= 2;
            return NULL;
        }

        *buffer_size *= 2;
    }
    return *buffer;
}

struct model *model_load_from_obj(const char *fname, bool color_support)
{
    FILE *fp = fopen(fname, "r");
    if (!fp)
    {
        fprintf(stderr, "ERROR: failed to load file \"%s\".\n", fname);
        return NULL;
    }

    // Create a new model
    struct model *model = model_init();

    int current_material = -1;

    // Read each line of the file
    int buffer_size = 128;
    char *buffer;

    if (!(buffer = malloc(buffer_size)))
    {
        fprintf(stderr, "ERROR: Memory allocation failure.\n");
        exit(1);
    }

    while (read_line(&buffer, &buffer_size, fp))
    {
        string_strip(buffer);

        char *bufferp = buffer;
        char *instr = str_chop_skip_empty(&bufferp, " ");

        if (!instr || instr[0] == '#')
            continue;

        if (strcmp(instr, "v") == 0)
        {
            float f1, f2, f3;

            if (!parse_float(&bufferp, &f1) || !parse_float(&bufferp, &f2) || !parse_float(&bufferp, &f3))
            {
                fprintf(stderr, "ERROR: invalid \"v\" instruction.\n");
                fclose(fp);
                model_free(model);
                return NULL;
            }

            vec3 vec;
            vec.x = f1;
            vec.y = f2;
            vec.z = f3;

            model_add_vertex(model, vec);
        }
        else if (strcmp(instr, "f") == 0)
        {
            // Parse face indexes
            int idx_count = 0;
            int idx_capacity = 1;
            int *idxs = malloc(idx_capacity * sizeof(int));

            int idx_read;
            while (parse_int(&bufferp, &idx_read))
            {
                if (idx_count == idx_capacity)
                {
                    idx_capacity *= 2;
                    idxs = realloc(idxs, idx_capacity * sizeof(int));
                }

                int idx = obj_derelativize_idx(idx_read, model->vertex_count);
                idxs[idx_count++] = idx;
            }

            if (idx_count < 3)
            {
                fprintf(stderr, "ERROR: invalid \"f\" instruction.\n");
                fclose(fp);
                free(idxs);
                model_free(model);
                return NULL;
            }

            // Triangularize face
            vec3 *vecs;
            if (!(vecs = malloc(idx_count * sizeof(vec3))))
            {
                fprintf(stderr, "ERROR: Memory allocation failure.\n");
                exit(1);
            }
            for (int i = 0; i < idx_count; ++i)
                vecs[i] = model->vertexes[idxs[i]];

            int *triangle_idxs;
            if (!(triangle_idxs = malloc((idx_count - 2) * 3 * sizeof(int))))
            {
                fprintf(stderr, "ERROR: Memory allocation failure.\n");
                exit(1);
            }

            triangularize(vecs, idx_count, triangle_idxs);

            for (int i = 0; i < idx_count - 2; ++i)
            {
                int i1 = idxs[triangle_idxs[3 * i]];
                int i2 = idxs[triangle_idxs[3 * i + 1]];
                int i3 = idxs[triangle_idxs[3 * i + 2]];

                model_add_face(model, i1, i2, i3, current_material);
            }

            free(idxs);
            free(vecs);
            free(triangle_idxs);
        }
        else if (color_support && strcmp(instr, "mtllib") == 0)
        {
            // Mutable copy of fname
            char *fname2;
            if (!(fname2 = malloc(strlen(fname) + 2)))
            {
                fprintf(stderr, "ERROR: Memory allocation failure.\n");
                exit(1);
            }
            strcpy(fname2, fname);

            // MTL file location
            const char *fname_dirname = dirname(fname2);
            size_t mtl_fname_size = strlen(fname_dirname) + strlen(bufferp) + 2;

            char *mtl_fname;
            if (!(mtl_fname = malloc(mtl_fname_size)))
            {
                free(fname2);
                fprintf(stderr, "ERROR: Memory allocation failure for MTL file name.\n");
                exit(1);
            }
            strcpy(mtl_fname, fname_dirname);
            strcat(mtl_fname, "/");
            strcat(mtl_fname, bufferp);

            fprintf(stderr, "NOTE: Reading \"%s\".\n", mtl_fname);

            model_load_materials_from_mtl(model, mtl_fname);

            free(fname2);
            free(mtl_fname);
        }
        else if (color_support && strcmp(instr, "usemtl") == 0)
        {
            const char *name = str_chop_skip_empty(&bufferp, " ");

            current_material = model_get_material_idx(model, name);
        }
    }

    free(buffer);
    fclose(fp);

    model_validate_idxs(model);
    return model;
}

struct model *model_load_from_stl(const char *fname)
{
    FILE *fp = fopen(fname, "rb");
    if (!fp)
    {
        fprintf(stderr, "ERROR: failed to load file \"%s\".\n", fname);
        return NULL;
    }

    // Create a new model
    struct model *model = model_init();

    // Read each line of the file
    char buffer[256];

    int current_material = -1;

    // Check this is an ASCII STL file
    // As the header of a binary STL could start with solid
    // we must also check the second line starts with facet
    bool is_ASCII = false;

    // Check first line starts with "solid"
    fgets(buffer, sizeof(buffer), fp);

    char *bufferp = buffer;
    char *instr = str_chop_skip_empty(&bufferp, " ");

    if (strcmp(instr, "solid") == 0)
    {
        // Check second line starts with "facet"
        fgets(buffer, sizeof(buffer), fp);

        char *bufferp = buffer;
        char *instr = str_chop_skip_empty(&bufferp, " ");

        if (strcmp(instr, "facet") == 0)
        {
            is_ASCII = true;
        }
    }

    if (is_ASCII)
    {
        while (fgets(buffer, sizeof(buffer), fp))
        {
            string_strip(buffer);

            char *bufferp = buffer;
            char *instr = str_chop_skip_empty(&bufferp, " ");

            // As we ignore normals only vertex definitions are required
            if (strcmp(instr, "vertex") == 0)
            {
                float f1, f2, f3;

                if (!parse_float(&bufferp, &f1) || !parse_float(&bufferp, &f2) || !parse_float(&bufferp, &f3))
                {
                    fprintf(stderr, "ERROR: invalid \"vertex\" instruction.\n");
                    fclose(fp);
                    model_free(model);
                    return NULL;
                }

                vec3 vec;
                vec.x = f1;
                vec.y = f3;
                vec.z = f2;

                model_add_vertex(model, vec);
            }
        }
    }
    else
    {
        // ASCII check will have moved the read pointer beyond the first block of data,
        // reset to byte 80, after the 80 byte header
        fseek(fp, 80, SEEK_SET);

        int facet_count_expected;
        int facet_count_actual = 0;

        char facet_count[4];
        if (fread(facet_count, sizeof(int), 1, fp) != 1)
        {
            fprintf(stderr, "ERROR: Failed to read facet count.\n");
            fclose(fp);
            model_free(model);
            return NULL;
        }
        // NOTE: Assuming little-endian hardware.
        memcpy(&facet_count_expected, facet_count, sizeof(int));

        // Read facet definitions, 50 bytes each, facet normal, 3 vertices, and a 2 byte spacer
        char buffer[50];
        size_t bytes_read;
        while((bytes_read = fread(buffer, sizeof(char), 50, fp)))
        {
            if (bytes_read < 50)
            {
                fprintf(stderr, "ERROR: Failed to read facet data.\n");
                fclose(fp);
                model_free(model);
                return NULL;
            }

            float facet[12];
            memcpy(&facet, buffer, sizeof(float[12]));

            for (int v_index = 0; v_index < 3; v_index++)
            {
                vec3 vec;
                vec.x = facet[3 + (v_index * 3)];
                vec.y = facet[5 + (v_index * 3)];
                vec.z = facet[4 + (v_index * 3)];

                model_add_vertex(model, vec);
            }

            ++facet_count_actual;
        }

        if (facet_count_expected != facet_count_actual)
        {
            fprintf(stderr, "WARN: imported facet count does not match expected facet count.\n");
        }
    }

    // For every 3 vertices create a face
    for (int i = 0; i < model->vertex_count; i += 3)
    {
        model_add_face(model, i, i + 2, i + 1, current_material);
    }

    fclose(fp);

    model_validate_idxs(model);
    return model;
}
