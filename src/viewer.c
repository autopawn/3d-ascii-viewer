#include "surface.h"
#include "model.h"

#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <argp.h>

// Screen clear function
#ifdef _WIN32
#include <conio.h>
#else
#include <stdio.h>
#define clrscr() printf("\e[1;1H\e[2J")
#endif

// Program documentation.
static char doc[] =
  "3d-ascii-viewer -- an OBJ 3D model format viewer for the terminal";

static char args_doc[] = "INPUT_FILE";

static struct argp_option options[] = {
  {"width",     'w',    "size",    0,   "Output width in characters" },
  {"height",    'h',    "size",    0,   "Output height in characters" },
  { 0 },
};

struct arguments
{
    int surface_width, surface_height;
    int arg_num;
    char *input_file;
};

/* Parse a single option. */
static error_t parse_opt (int key, char *arg, struct argp_state *state)
{
  /* Get the input argument from argp_parse, which we
     know is a pointer to our arguments structure. */
  struct arguments *args = state->input;

  switch (key)
    {
        case 'w':
            args->surface_width = atoi(arg);
            break;

        case 'h':
            args->surface_height = atoi(arg);
            break;

        case ARGP_KEY_ARG:

            // Handle too many arguments
            if (state->arg_num >= 1)
                argp_usage(state);

            args->input_file = arg;

            state->arg_num++;
            break;

        case ARGP_KEY_END:

            // Handle not enough arguments
            if (state->arg_num < 1)
                argp_usage (state);

            break;

        default:
            return ARGP_ERR_UNKNOWN;
    }

    return 0;
}

static struct argp argp = {options, parse_opt, args_doc, doc};

// Translate from the [-1,1]^3 cube to the screen surface.
vec3 vec3_to_screen(vec3 v)
{
    v.x = 0.5 + 0.5 * v.x;
    v.y = 0.5 - 0.5 * v.y;
    v.z = 0.5 + 0.5 * v.z;
    return v;
}

vec3 vec3_rotate_y(float cos, float sin, vec3 v)
{
    float x = v.x * cos - v.z * sin;
    float z = v.x * sin + v.z * cos;
    v.x = x;
    v.z = z;
    return v;
}

vec3 vec3_rotate_x(float cos, float sin, vec3 v)
{
    float y = v.y * cos - v.z * sin;
    float z = v.y * sin + v.z * cos;
    v.y = y;
    v.z = z;
    return v;
}

int main(int argc, char *argv[])
{
    struct arguments args = {0};

    // Default values
    args.surface_width = 80;
    args.surface_height = 50;

    argp_parse(&argp, argc, argv, 0, 0, &args);

    struct model *model = model_load_from_obj(args.input_file);
    if (!model)
        return 1;
    model_normalize(model);

    struct surface *surface = surface_init(args.surface_width, args.surface_height);
    if (!surface)
        return 1;

    int t = 0;
    while (1)
    {
        float elev = 0.125 * 3.14159265358979323846 * (sinf(0.05 * t) - 1);
        float elev_cos = cosf(elev);
        float elev_sin = sinf(elev);

        float az = 0.12 * t;
        float az_cos = cosf(az);
        float az_sin = sinf(az);

        surface_clear(surface);

        for (int f = 0; f < model->faces_count; ++f)
        {
            int i1 = model->idxs[3 * f + 0];
            int i2 = model->idxs[3 * f + 1];
            int i3 = model->idxs[3 * f + 2];

            vec3 v1 = model->vertexes[i1];
            vec3 v2 = model->vertexes[i2];
            vec3 v3 = model->vertexes[i3];

            v1 = vec3_rotate_y(az_cos, az_sin, v1);
            v2 = vec3_rotate_y(az_cos, az_sin, v2);
            v3 = vec3_rotate_y(az_cos, az_sin, v3);

            v1 = vec3_rotate_x(elev_cos, elev_sin, v1);
            v2 = vec3_rotate_x(elev_cos, elev_sin, v2);
            v3 = vec3_rotate_x(elev_cos, elev_sin, v3);

            struct triangle tri = {0};
            tri.p1 = vec3_to_screen(v1);
            tri.p2 = vec3_to_screen(v2);
            tri.p3 = vec3_to_screen(v3);

            surface_draw_triangle(surface, tri);
        }

        clrscr();
        surface_print(stdout, surface);
        usleep(100000);
        t++;
    }

    surface_free(surface);
    model_free(model);
}
