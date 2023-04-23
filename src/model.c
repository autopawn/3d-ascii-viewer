#include "model.h"

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

static struct model *model_init(void)
{
    struct model *model = malloc(sizeof(*model));

    model->faces_capacity = 1;
    model->idxs = malloc(model->faces_capacity * 3 * sizeof(*model->idxs));
    assert(model->idxs);
    model->faces_count = 0;

    model->vertex_capacity = 1;
    model->vertexes = malloc(model->vertex_capacity * sizeof(*model->vertexes));
    assert(model->vertexes);
    model->vertex_count = 0;
}

static void model_add_vertex(struct model *model, vec3 vec)
{
    if (model->vertex_count == model->vertex_capacity)
    {
        model->vertex_capacity *= 2;
        model->vertexes = realloc(model->vertexes, model->vertex_capacity * sizeof(*model->vertexes));
        assert(model->vertexes);
    }

    model->vertexes[model->vertex_count] = vec;
    model->vertex_count++;
}

static int relativize_idx(int i, int n)
{
    if (i < -n || i == 0 || i > n)
    {
        fprintf(stderr, "WARN: Invalid vertex index %d.\n", i);
        return 0;
    }

    if (i > 0)
        return i - 1;
    if (i < 0)
        return n + i;
}

static void model_add_face(struct model *model, int idx1, int idx2, int idx3)
{
    if (model->faces_count == model->faces_capacity)
    {
        model->faces_capacity *= 2;
        model->idxs = realloc(model->idxs, 3 * model->faces_capacity * sizeof(*model->idxs));
        assert(model->idxs);
    }
    model->idxs[3 * model->faces_count + 0] = relativize_idx(idx1, model->vertex_count);
    model->idxs[3 * model->faces_count + 1] = relativize_idx(idx2, model->vertex_count);
    model->idxs[3 * model->faces_count + 2] = relativize_idx(idx3, model->vertex_count);

    model->faces_count++;
}

void model_free(struct model *model)
{
    free(model->vertexes);
    free(model->idxs);
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

struct model *model_init_from_obj(const char *fname)
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

    while (fgets(buffer, sizeof(buffer), fp))
    {
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
                return model;
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
                return model;
            }

            while (parse_int(&bufferp, &i3))
            {
                model_add_face(model, i1, i2, i3);
                // Shift for possible new triangular face
                i2 = i3;
            }
        }
    }

    fclose(fp);
    return model;
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