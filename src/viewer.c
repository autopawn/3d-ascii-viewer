#include "surface.h"
#include "model.h"

#include <ncurses.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <sys/time.h>
#include <argp.h>

// Program documentation.
static char doc[] =
  "3d-ascii-viewer -- an OBJ 3D model format viewer for the terminal";

static char args_doc[] = "INPUT_FILE";

static struct argp_option options[] = {
  {"width",        'w',    "size",     0,   "Output width in characters" },
  {"height",       'h',    "size",     0,   "Output height in characters" },
  {"fps",          'f',    "frames",   0,   "Frames per second." },
  {"duration",     'd',    "seconds",  0,   "Stop the program after this many seconds." },
  {"aspect-ratio", 'a',    "ratio",    0,   "Display assuming this height/width ratio for terminal characters." },
  {"stretch",      's',    NULL,       0,   "Stretch the model, regardless of the aspect-ratio." },
  { 0 },
};

struct arguments
{
    int surface_width, surface_height, fps;
    bool finite;
    float duration;
    float aspect_ratio;
    bool stretch;

    int arg_num;
    char *input_file;
};

/* Parse a single option. */
static error_t parse_opt(int key, char *arg, struct argp_state *state)
{
  /* Get the input argument from argp_parse, which we
     know is a pointer to our arguments structure. */
  struct arguments *args = state->input;

  switch (key)
    {
        case 'w':
            args->surface_width = strtol(arg, NULL, 10);
            if (errno || args->surface_width <= 0)
            {
                fprintf(stderr, "ERROR: Invalid width: %s\n", arg);
                exit(1);
            }
            break;

        case 'h':
            args->surface_height = strtol(arg, NULL, 10);
            if (errno || args->surface_height <= 0)
            {
                fprintf(stderr, "ERROR: Invalid height: %s\n", arg);
                exit(1);
            }
            break;

        case 'f':
            args->fps = strtol(arg, NULL, 10);
            if (errno || args->fps <= 0)
            {
                fprintf(stderr, "ERROR: Invalid FPS: %s\n", arg);
                exit(1);
            }
            break;

        case 'd':
            args->duration = strtof(arg, NULL);
            if (errno || args->duration < 0)
            {
                fprintf(stderr, "ERROR: Invalid duration: %s\n", arg);
                exit(1);
            }
            args->finite = true;
            break;

        case 'a':
            args->aspect_ratio = strtof(arg, NULL);
            if (errno || args->aspect_ratio <= 0)
            {
                fprintf(stderr, "ERROR: Invalid aspect-ratio: %s\n", arg);
                exit(1);
            }
            break;

       case 's':
            args->stretch = true;
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

// The argp parser
static struct argp argp = {options, parse_opt, args_doc, doc};

// Get current time in microseconds
unsigned long long get_current_useconds(void)
{
    unsigned long long ret;
    struct timeval time;

    gettimeofday(&time, NULL);
    ret = 1000000 * time.tv_sec;
    ret += time.tv_usec;

    return ret;
}

// Wait until frame ends function
static void tick(unsigned long long *last_target, unsigned long long frame_duration)
{
    unsigned long long current, target, delta;

    current = get_current_useconds();
    target = *last_target + frame_duration;
    if (current < target)
    {
        delta = target - current;
        if (delta > frame_duration)
            delta = frame_duration;
        usleep(delta);
        *last_target = current + delta;
    }
    else
    {
        *last_target = current;
    }
}

// Translate from the [-1,1]^3 cube to the screen surface.
static vec3 vec3_to_surface(const struct surface *surface, vec3 v)
{
    v.x = 0.5 * surface->logical_size_x + 0.5 * v.x;
    v.y = 0.5 * surface->logical_size_y - 0.5 * v.y;
    v.z = 0.5 + 0.5 * v.z;
    return v;
}

static vec3 vec3_rotate_y(float cos, float sin, vec3 v)
{
    float x = v.x * cos - v.z * sin;
    float z = v.x * sin + v.z * cos;
    v.x = x;
    v.z = z;
    return v;
}

static vec3 vec3_rotate_x(float cos, float sin, vec3 v)
{
    float y = v.y * cos - v.z * sin;
    float z = v.y * sin + v.z * cos;
    v.y = y;
    v.z = z;
    return v;
}

static void surface_draw_model(struct surface *surface, const struct model *model, float azimuth, float altitude)
{
    float elev_cos = cosf(altitude);
    float elev_sin = sinf(altitude);

    float az_cos = cosf(azimuth);
    float az_sin = sinf(azimuth);

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
        tri.p1 = vec3_to_surface(surface, v1);
        tri.p2 = vec3_to_surface(surface, v2);
        tri.p3 = vec3_to_surface(surface, v3);

        surface_draw_triangle(surface, tri);
    }
}

static void compute_surface_logical_size(const struct model *model, float aspect_rel,
        unsigned int width, unsigned int height, float *out_x, float *out_y)
{
    // Find width required by the model
    // (height is assumed 1.0, because the model is normalized, and all azimuth and elevation rotations are considered).
    float required_y = 1.0;
    float required_x = 0.0;
    for (int i = 0; i < model->vertex_count; ++i)
    {
        vec3 v = model->vertexes[i];

        float dist_x = sqrtf(v.x * v.x + v.z * v.z);
        if (dist_x > required_x)
            required_x = dist_x;
    }

    // Screen width / height
    float screen_aspect_rel = width / (height * aspect_rel);

    if (screen_aspect_rel * required_y >= 1.0 * required_x)
    {
        *out_x = required_y * screen_aspect_rel;
        *out_y = required_y;
    }
    else
    {
        *out_x = required_x;
        *out_y = required_x / screen_aspect_rel;
    }
}

int main(int argc, char *argv[])
{
    struct arguments args = {0};

    // Default values
    args.surface_width = 80;
    args.surface_height = 40;
    args.aspect_ratio = 1.8;
    args.stretch = false;
    args.fps = 20;
    args.duration = 0;

    argp_parse(&argp, argc, argv, 0, 0, &args);

    struct model *model = model_load_from_obj(args.input_file);
    if (!model)
        return 1;
    if (model->vertex_count == 0)
    {
        fprintf(stderr, "ERROR: Could not read model vertexes.\n");
        exit(1);
    }
    if (model->faces_count == 0)
    {
        fprintf(stderr, "ERROR: Could not read model faces.\n");
        exit(1);
    }
    model_normalize(model);

    float surface_size_x = 1.0, surface_size_y = 1.0;

    if (!args.stretch)
        compute_surface_logical_size(model, args.aspect_ratio, args.surface_width, args.surface_height,
                &surface_size_x, &surface_size_y);

    struct surface *surface = surface_init(args.surface_width, args.surface_height,
            surface_size_x, surface_size_y);
    if (!surface)
        return 1;

    // Initialize clock
    unsigned long long frame_duration = (1000000 + args.fps - 1)/args.fps;
    unsigned long long start = get_current_useconds();
    unsigned long long clock = start;
    unsigned long long duration = (unsigned long long) (args.duration * 1000000);

    // Start curses mode
    initscr();
    noecho();

    timeout(0);
    int t = 0;
    while (1)
    {
        surface_clear(surface);

        float time = t * (frame_duration / 1000000.0);
        float altitude = 0.125 * 3.14159265358979323846 * (sinf(0.5 * time) - 1);
        float azimuth = 2.0 * time;

        surface_draw_model(surface, model, azimuth, altitude);

        // Print surface
        move(0, 0);
        surface_printw(surface);
        refresh();

        if ((args.finite && clock - start >= duration) || getch() != ERR)
            break;

        tick(&clock, frame_duration);

        t++;
    }

    // End curses mode
    endwin();

    // One last print, in case the user wants to copy the last frame after the given duration.
    surface_print(stdout, surface);

    // Free memory
    surface_free(surface);
    model_free(model);
}
