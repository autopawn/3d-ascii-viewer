#include "surface.h"
#include "model.h"

#include <errno.h>
#include <ncurses.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>

// Program description
static const char *PROGRAM_NAME = "3d-ascii-viewer";
static const char *PROGRAM_DESCRIPTION = "an OBJ 3D model format viewer for the terminal";

// Program documentation.
static void output_usage(int argc, char *argv[])
{
    printf("Usage: %s [OPTION...] INPUT_FILE\n", argv[0]);
    printf("%s -- %s\n", PROGRAM_NAME, PROGRAM_DESCRIPTION);
    printf("\n");
    printf("  -w <size>         Output width in characters\n");
    printf("  -h <size>         Output height in characters\n");
    printf("  -d <seconds>      Stop the program after this many seconds.\n");
    printf("  -f <frames>       Frames per second.\n");
    printf("  -a <ratio>        Display assuming this height/width ratio for terminal\n");
    printf("                    characters.\n");
    printf("  -s                Stretch the model, regardless of the height/width ratio.\n");
    printf("                    for terminal characters.\n");
    printf("\n");
    printf("  --snap <az> <al>  Output a single snap to stdout, with the given azimuth\n");
    printf("                    and altitude angles, in degrees.\n");
    printf("\n");
    printf("  -?, --help        Give this help list\n");
    printf("\n");

    exit(1);
}

static void output_description(int argc, char *argv[])
{
    printf("Usage: %s [OPTION...] INPUT_FILE\n", argv[0]);
    printf("%s -- %s\n", PROGRAM_NAME, PROGRAM_DESCRIPTION);
    printf("Try `%s --help' for more information.\n", argv[0]);

    exit(1);
}

struct arguments
{
    int surface_width, surface_height, fps;
    bool finite;
    float duration;
    float aspect_ratio;
    bool stretch;

    bool snap_mode;
    float azimuth, altitude;

    int arg_num;
    char *input_file;
};

static void parse_arguments(int argc, char *argv[], struct arguments *args)
{
    for (int i = 1; i < argc; ++i)
    {
        if (!strcmp(argv[i], "-?") || !strcmp(argv[i], "--help"))
        {
            output_usage(argc, argv);
        }
        else if (!strcmp(argv[i], "-w"))
        {
            if (i >= argc - 1)
                output_usage(argc, argv);
            args->surface_width = strtol(argv[++i], NULL, 10);
            if (errno || args->surface_width <= 0)
            {
                fprintf(stderr, "ERROR: Invalid width: %s\n", argv[i]);
                exit(1);
            }
        }
        else if (!strcmp(argv[i], "-h"))
        {
            if (i >= argc - 1)
                output_usage(argc, argv);
            args->surface_height = strtol(argv[++i], NULL, 10);
            if (errno || args->surface_height <= 0)
            {
                fprintf(stderr, "ERROR: Invalid height: %s\n", argv[i]);
                exit(1);
            }
        }
        else if (!strcmp(argv[i], "-f"))
        {
            if (i >= argc - 1)
                output_usage(argc, argv);
            args->fps = strtol(argv[++i], NULL, 10);
            if (errno || args->fps <= 0)
            {
                fprintf(stderr, "ERROR: Invalid FPS: %s\n", argv[i]);
                exit(1);
            }
        }
        else if (!strcmp(argv[i], "-d"))
        {
            if (i >= argc - 1)
                output_usage(argc, argv);
            args->duration = strtof(argv[++i], NULL);
            if (errno || args->duration < 0)
            {
                fprintf(stderr, "ERROR: Invalid duration: %s\n", argv[i]);
                exit(1);
            }
            args->finite = true;
        }
        else if (!strcmp(argv[i], "-a"))
        {
            if (i >= argc - 1)
                output_usage(argc, argv);
            args->aspect_ratio = strtof(argv[++i], NULL);
            if (errno || args->aspect_ratio <= 0)
            {
                fprintf(stderr, "ERROR: Invalid aspect-ratio: %s\n", argv[i]);
                exit(1);
            }
        }
        else if (!strcmp(argv[i], "-s"))
        {
            args->stretch = true;
        }
        else if (!strcmp(argv[i], "--snap"))
        {
            if (i >= argc - 2)
                output_usage(argc, argv);
            args->snap_mode = true;
            args->azimuth = strtof(argv[++i], NULL);
            if (errno || args->duration < 0)
            {
                fprintf(stderr, "ERROR: Invalid azimuth: %s\n", argv[i]);
                exit(1);
            }
            args->altitude = strtof(argv[++i], NULL);
            if (errno || args->duration < 0)
            {
                fprintf(stderr, "ERROR: Invalid altitude: %s\n", argv[i]);
                exit(1);
            }
        }
        else if (argv[i][0] == '-')
        {
            fprintf(stderr, "ERROR: Invalid option: %s\n", argv[i]);
            exit(1);
        }
        else
        {
            // Handle too many arguments
            if (args->input_file)
                output_usage(argc, argv);

            args->input_file = argv[i];
        }
    }

    // Handle too few arguments
    if (!args->input_file)
        output_usage(argc, argv);
}

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

static void surface_draw_model(struct surface *surface, const struct model *model, float azimuth,
        float altitude)
{
    float elev_cos = cosf(-altitude);
    float elev_sin = sinf(-altitude);

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
    // (height is assumed 1.0, because the model is normalized, and all azimuth and altitude
    // rotations are considered).
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
    if (argc == 1)
        output_description(argc, argv);

    // Argument default values
    struct arguments args = {0};
    args.input_file = NULL;
    args.surface_width = 80;
    args.surface_height = 40;
    args.aspect_ratio = 1.8;
    args.stretch = false;
    args.fps = 20;
    args.duration = 0;
    args.snap_mode = false;
    args.azimuth = 0.0;
    args.altitude = 0.0;

    parse_arguments(argc, argv, &args);

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


    if (args.snap_mode)
    {
        float azimuth = 3.14159265358979323846 * args.azimuth / 180.0;
        float altitude = 3.14159265358979323846 * args.altitude / 180.0;
        surface_draw_model(surface, model, azimuth, altitude);

        surface_print(stdout, surface);
    }
    else
    {
        // Start curses mode
        initscr();
        noecho();

        timeout(0);
        int t = 0;
        while (1)
        {
            surface_clear(surface);

            float time = t * (frame_duration / 1000000.0);
            float azimuth = 2.0 * time;
            float altitude = 0.125 * 3.14159265358979323846 * (1 - sinf(0.5 * time));

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
    }

    // Free memory
    surface_free(surface);
    model_free(model);
}
