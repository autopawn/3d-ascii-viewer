#include "model.h"

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

void model_bounding_box(const struct model *model, vec3 *minp, vec3 *maxp)
{
    vec3 min = {0};
    vec3 max = {0};

    if (model->vertex_count > 0)
    {
        min = model->vertexes[0];
        max = model->vertexes[0];
    }

    for (int i = 0; i < model->vertex_count; i++)
    {
        vec3 v = model->vertexes[i];

        if (v.x < min.x)
            min.x = v.x;
        if (v.y < min.y)
            min.y = v.y;
        if (v.z < min.z)
            min.z = v.z;

        if (v.x > max.x)
            max.x = v.x;
        if (v.y > max.y)
            max.y = v.y;
        if (v.z > max.z)
            max.z = v.z;
    }

    *minp = min;
    *maxp = max;
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
    vec3 min, max, center;

    model_bounding_box(model, &min, &max);

    center.x = (min.x + max.x) / 2.0;
    center.y = (min.y + max.y) / 2.0;
    center.z = (min.z + max.z) / 2.0;

    for (int i = 0; i < model->vertex_count; ++i)
    {
        model->vertexes[i].x -= center.x;
        model->vertexes[i].y -= center.y;
        model->vertexes[i].z -= center.z;
    }

    float max_mag = 0.0;
    for (int i = 0; i < model->vertex_count; ++i)
    {
        float mag = vec3_mag(model->vertexes[i]);

        if (mag > max_mag)
            max_mag = mag;
    }

    float scale = (max_mag == 0) ? 1.0 : 1.0 / max_mag;
    for (int i = 0; i < model->vertex_count; ++i)
    {
        model->vertexes[i].x *= scale;
        model->vertexes[i].y *= scale;
        model->vertexes[i].z *= scale;
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

    // Read each line of the file
    char buffer[256];

    int current_material = -1;

    while (fgets(buffer, sizeof(buffer), fp))
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
            int i1, i2, i3;

            if (!parse_int(&bufferp, &i1) || !parse_int(&bufferp, &i2))
            {
                fprintf(stderr, "ERROR: invalid \"f\" instruction.\n");
                fclose(fp);
                model_free(model);
                return NULL;
            }

            while (parse_int(&bufferp, &i3))
            {
                int i1d, i2d, i3d;

                i1d = obj_derelativize_idx(i1, model->vertex_count);
                i2d = obj_derelativize_idx(i2, model->vertex_count);
                i3d = obj_derelativize_idx(i3, model->vertex_count);

                model_add_face(model, i1d, i2d, i3d, current_material);
                // Shift for possible new triangular face
                i2 = i3;
            }
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

    fclose(fp);

    model_validate_idxs(model);
    return model;
}
