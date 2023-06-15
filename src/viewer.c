#include "surface.h"
#include "model.h"

#include <errno.h>
#include <ncurses.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>

static char *DEFAULT_LUM_OPTIONS = ".,':;!+*=#$@";
static const float PI = 3.1415926536;
static const float GOLDEN_RATIO = 1.6180339887;

static const float INTERACTIVE_ZOOM_MIN = 5;
static const float INTERACTIVE_ZOOM_MAX = 1000;

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
    printf("  -c <chars>        Provide alternate luminescence characters (from less to\n");
    printf("                    more light).\n");
    printf("  -s                Stretch the model, regardless of the height/width ratio.\n");
    printf("                    for terminal characters.\n");
    printf("  -t                Allow the animation to reach maximum elevation.\n");
    printf("  -l                Don't rotate the light with the model.\n");
    printf("  -X, -Y, -Z        Invert respective axes.\n");
    printf("  -z <zoom>         Change zoom level (default: 100).\n");
    printf("\n");
    printf("  --color           Display with colors.\n");
    printf("                    The OBJ format relies on the companion MTL files.\n");
    printf("\n");
    printf("  --snap <az> <al>  Output a single snap to stdout, with the given azimuth\n");
    printf("                    and altitude angles, in degrees.\n");
    printf("\n");
    printf("  --interactive     Manually rotate the camera.\n");
    printf("                    Controls: ARROW KEYS, '-', '+'\n");
    printf("                    Alt-controls: H, J, K, L, A, S\n");
    printf("                    Quit: Q    Toggle Hud: T\n");
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
    bool top_elevation;
    bool static_light;
    char *lum_chars;
    bool invert_x, invert_y, invert_z;

    bool color_support;

    bool snap_mode;
    float azimuth, altitude;
    float zoom;

    bool interactive;

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
        else if (!strcmp(argv[i], "-c"))
        {
            if (i >= argc - 1)
                output_usage(argc, argv);
            args->lum_chars = argv[++i];
            if (args->lum_chars[0] == '\0')
            {
                fprintf(stderr, "ERROR: At least one luminescence character must be provided.\n");
                exit(1);
            }
        }
        else if (!strcmp(argv[i], "-s"))
        {
            args->stretch = true;
        }
        else if (!strcmp(argv[i], "-t"))
        {
            args->top_elevation = true;
        }
        else if (!strcmp(argv[i], "-l"))
        {
            args->static_light = true;
        }
        else if (!strcmp(argv[i], "-X"))
        {
            args->invert_x = true;
        }
        else if (!strcmp(argv[i], "-Y"))
        {
            args->invert_y = true;
        }
        else if (!strcmp(argv[i], "-Z"))
        {
            args->invert_z = true;
        }
        else if (!strcmp(argv[i], "-z"))
        {
            if (i >= argc - 1)
                output_usage(argc, argv);
            args->zoom = strtof(argv[++i], NULL);
            if (errno || args->zoom <= 0)
            {
                fprintf(stderr, "ERROR: Invalid zoom: %s\n", argv[i]);
                exit(1);
            }
        }
        else if (!strcmp(argv[i], "--color"))
        {
            args->color_support = true;
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
        else if (!strcmp(argv[i], "--interactive"))
        {
            args->interactive = true;
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
static vec3 vec3_to_surface(const struct surface *surface, vec3 v, float zoom)
{
    v.x = 0.5 * surface->logical_size_x + 0.5 * v.x * zoom;
    v.y = 0.5 * surface->logical_size_y - 0.5 * v.y * zoom;
    v.z = 0.5 + 0.5 * v.z * zoom;
    return v;
}

static char char_from_normal(vec3 normal, vec3 light_normal, const char *lum_chars, int lum_count)
{
    float sim = vec3_cos_similarity(normal, light_normal, 1.0, 1.0) * 0.5 + 0.5;
    int p = (int) roundf((lum_count - 1) * sim);
    if (p < 0)
        p = 0;
    if (p >= lum_count)
        p = lum_count - 1;
    return lum_chars[p];
}

static void terminal_init_colors(const struct model *model)
{
    const int MINIMUM_COLOR_VALUE_SUM = 140;

    for (int i = 0; i < model->materials_count; ++i)
    {
        int color = i + 1;

        if (color >= COLORS || color >= COLOR_PAIRS)
        {
            fprintf(stderr, "WARN: Terminal doesn't support enough colors for all materials.\n");
            return;
        }

        int r = (int)(model->materials[i].Kd_r * 1000);
        int g = (int)(model->materials[i].Kd_g * 1000);
        int b = (int)(model->materials[i].Kd_b * 1000);

        if (r + g + b < MINIMUM_COLOR_VALUE_SUM)
        {
            int rem = MINIMUM_COLOR_VALUE_SUM - r + g + b;
            r += (rem + 2)/3;
            g += (rem + 2)/3;
            b += (rem + 2)/3;
        }

        if (r > 1000)
            r = 1000;
        if (r < 0)
            r = 0;
        if (g > 1000)
            g = 1000;
        if (g < 0)
            g = 0;
        if (b > 1000)
            b = 1000;
        if (b < 0)
            b = 0;

        init_color(color, (short)r, (short)g, (short)b);
        init_pair(color, color, 0);
    }
}

static void surface_draw_model(struct surface *surface, const struct model *model, float azimuth,
        float altitude, float zoom, bool static_light, const char *lum_chars, bool color_support)
{
    int lum_count = strlen(lum_chars);

    float alt_cos = cosf(-altitude);
    float alt_sin = sinf(-altitude);

    float az_cos = cosf(azimuth);
    float az_sin = sinf(azimuth);

    vec3 light = static_light ? (vec3){0.75, -1.0, -0.5} : (vec3){1, -1, 0};
    light = vec3_normalize(light);

    for (int f = 0; f < model->faces_count; ++f)
    {
        int i1 = model->faces[f].idxs[0];
        int i2 = model->faces[f].idxs[1];
        int i3 = model->faces[f].idxs[2];

        vec3 v1 = model->vertexes[i1];
        vec3 v2 = model->vertexes[i2];
        vec3 v3 = model->vertexes[i3];

        triangle tri = {.p1 = v1, .p2 = v2, .p3 = v3};

        tri.p1 = vec3_rotate_y(az_cos, az_sin, tri.p1);
        tri.p2 = vec3_rotate_y(az_cos, az_sin, tri.p2);
        tri.p3 = vec3_rotate_y(az_cos, az_sin, tri.p3);

        tri.p1 = vec3_rotate_x(alt_cos, alt_sin, tri.p1);
        tri.p2 = vec3_rotate_x(alt_cos, alt_sin, tri.p2);
        tri.p3 = vec3_rotate_x(alt_cos, alt_sin, tri.p3);

        tri.p1 = vec3_to_surface(surface, tri.p1, zoom);
        tri.p2 = vec3_to_surface(surface, tri.p2, zoom);
        tri.p3 = vec3_to_surface(surface, tri.p3, zoom);

        char c;
        if (static_light)
        {
            triangle tri_ini = {.p1 = v1, .p2 = v2, .p3 = v3};
            tri_ini.p1 = vec3_to_surface(surface, tri_ini.p1, zoom);
            tri_ini.p2 = vec3_to_surface(surface, tri_ini.p2, zoom);
            tri_ini.p3 = vec3_to_surface(surface, tri_ini.p3, zoom);

            c = char_from_normal(vec3_neg(triangle_normal(&tri_ini)), light, lum_chars, lum_count);
        }
        else
        {
            c = char_from_normal(vec3_neg(triangle_normal(&tri)), light, lum_chars, lum_count);
        }

        surface_draw_triangle(surface, tri, true, c, color_support ? model->faces[f].material : -1);
    }
}

// Model radius only in X and Z.
static float model_xz_rad(const struct model *model)
{
    float rad = 0.0;
    for (int i = 0; i < model->vertex_count; ++i)
    {
        vec3 v = model->vertexes[i];

        float dist_xz = sqrtf(v.x * v.x + v.z * v.z);
        if (dist_xz > rad)
            rad = dist_xz;
    }
    return rad;
}

static struct surface *create_surface(const struct model *model, int arg_surface_w, int arg_surface_h,
        float char_aspect_ratio, bool stretch)
{
    // Logical size required by the model
    float required_y = 1.0;
    float required_x = model_xz_rad(model);
    // Surface logical size
    float surface_size_x, surface_size_y;
    // Surface size in characters
    int surface_w, surface_h;

    // User provided arguments override screen size given by ncurses
    getmaxyx(stdscr, surface_h, surface_w);
    if (arg_surface_h)
        surface_h = arg_surface_h;
    if (arg_surface_w)
        surface_w = arg_surface_w;

    if (stretch)
    {
        surface_size_x = required_x;
        surface_size_y = required_y;
    }
    else
    {
        // Screen width / height
        float screen_aspect_rel = surface_w / (surface_h * char_aspect_ratio);

        if (screen_aspect_rel * required_y >= 1.0 * required_x)
        {
            surface_size_x = required_y * screen_aspect_rel;
            surface_size_y = required_y;
        }
        else
        {
            surface_size_x = required_x;
            surface_size_y = required_x / screen_aspect_rel;
        }
    }

    return surface_init(surface_w, surface_h, surface_size_x, surface_size_y);
}

const char *get_file_extension(const char *filename)
{
    const char *dot = strrchr(filename, '.');
    if (!dot || dot == filename) return NULL;
    return dot + 1;
}

int main(int argc, char *argv[])
{
    if (argc == 1)
        output_description(argc, argv);

    // Argument default values
    struct arguments args = {0};
    args.input_file = NULL;
    args.surface_width = 0;
    args.surface_height = 0;
    args.aspect_ratio = 1.8;
    args.stretch = false;
    args.fps = 20;
    args.duration = 0;
    args.top_elevation = false;
    args.static_light = false;
    args.lum_chars = DEFAULT_LUM_OPTIONS;
    args.invert_x = false;
    args.invert_y = false;
    args.invert_z = false;
    args.zoom = 100;

    args.color_support = false;

    args.snap_mode = false;
    args.azimuth = 0.0;
    args.altitude = 0.0;

    args.interactive = false;

    parse_arguments(argc, argv, &args);

    struct model *model;

    const char *fileExt = get_file_extension(args.input_file);
    if (fileExt == NULL)
    {
        fprintf(stderr, "ERROR: Input file has no extension.\n");
        exit(1);
    }
    else if (strcmp(fileExt, "obj") == 0)
    {
        if (!(model = model_load_from_obj(args.input_file, args.color_support)))
            return 1;
        model_invert_z(model); // Required by the OBJ format.
    }
    else if (strcmp(fileExt, "stl") == 0)
    {
        if (args.color_support)
        {
            fprintf(stderr, "WARN: Colors are not supported in STL format.\n");
        }
        if (!(model = model_load_from_stl(args.input_file)))
            return 1;
    }
    else
    {
        fprintf(stderr, "ERROR: Input file has unsupported extension.\n");
        exit(1);
    }

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

    // Invert axes as required by the options
    if (args.invert_x)
        model_invert_x(model);
    if (args.invert_y)
        model_invert_y(model);
    if (args.invert_z)
        model_invert_z(model);

    // Starting curses is required to get the screen size
    struct surface *surface;
    initscr();
    surface = create_surface(model, args.surface_width, args.surface_height, args.aspect_ratio, args.stretch);
    endwin(); // End curses mode
    if (!surface)
        return 1;

    if (args.color_support)
    {
        if (has_colors() == FALSE)
        {
            endwin();
            fprintf(stderr, "ERROR: Terminal does not support colors.\n");
            exit(1);
        }
        if (can_change_color() == FALSE)
        {
            endwin();
            fprintf(stderr, "ERROR: Terminal does not support changing colors.\n");
            exit(1);
        }
        start_color();
        terminal_init_colors(model);
    }

    // Initialize clock
    unsigned long long frame_duration = (1000000 + args.fps - 1)/args.fps;
    unsigned long long start = get_current_useconds();
    unsigned long long clock = start;
    unsigned long long duration = (unsigned long long) (args.duration * 1000000);

    if (args.snap_mode)
    {
        float azimuth = PI * args.azimuth / 180.0;
        float altitude = PI * args.altitude / 180.0;
        float zoom = args.zoom / 100.0;
        surface_draw_model(surface, model, azimuth, altitude, zoom, args.static_light,
                args.lum_chars, args.color_support);

        surface_print(stdout, surface);
    }
    else if (args.interactive)
    {
        initscr();
        noecho();
        curs_set(0);
        timeout(-1);
        keypad(stdscr, TRUE); // read special keys.

        const float angle_move = 15.0;
        float azimuth_deg = 0.0;
        float altitude_deg = 0.0;
        float zoom = args.zoom;

        bool hud = true;

        while (1)
        {
            surface_clear(surface);

            float azimuth = PI * azimuth_deg / 180;
            float altitude = PI * altitude_deg / 180;

            surface_draw_model(surface, model, azimuth, altitude, zoom / 100.0,
                    args.static_light, args.lum_chars, args.color_support);

            // Print surface
            move(0, 0);
            surface_printw(surface);
            if (hud)
            {
                move(0, 0);
                printw("zo:%4.0f", zoom);
                move(1, 0);
                printw("az: %3.0f", azimuth_deg);
                move(2, 0);
                printw("al: %3.0f", altitude_deg);
            }
            refresh();

            int key = getch();

            if (key == KEY_RESIZE)
            {
                surface_free(surface);
                surface = create_surface(model, args.surface_width, args.surface_height,
                        args.aspect_ratio, args.stretch);
                if (!surface)
                    return 1;
            }

            if (key == 'q')
                break;
            if (key == 't')
                hud = !hud;
            if (key == 'h' || key == KEY_LEFT)
                azimuth_deg += angle_move;
            if (key == 'l' || key == KEY_RIGHT)
                azimuth_deg -= angle_move;
            if (key == 'j' || key == KEY_DOWN)
                altitude_deg -= angle_move;
            if (key == 'k' || key == KEY_UP)
                altitude_deg += angle_move;
            if (key == '-' || key == 'a')
                zoom -= 5;
            if (key == '+' || key == 's')
                zoom += 5;

            if (azimuth_deg < 0)
                azimuth_deg += 360;
            if (azimuth_deg >= 360)
                azimuth_deg -= 360;

            if (altitude_deg > 180)
                altitude_deg = 180;
            if (altitude_deg < -180)
                altitude_deg = -180;

            if (zoom > INTERACTIVE_ZOOM_MAX)
                zoom = INTERACTIVE_ZOOM_MAX;
            if (zoom < INTERACTIVE_ZOOM_MIN)
                zoom = INTERACTIVE_ZOOM_MIN;
        }

        endwin();
    }
    else
    {
        initscr();
        noecho();
        curs_set(0);
        timeout(0);

        int t = 0;
        while (1)
        {
            surface_clear(surface);

            float time = t * (frame_duration / 1000000.0);

            const float az_speed = 2.0;
            const float al_speed = GOLDEN_RATIO * 0.25;
            float azimuth = az_speed * time;
            float altitude = (args.top_elevation ? 0.25 : 0.125) * PI * (1 - sinf(al_speed * time));
            float zoom = args.zoom / 100.0;

            surface_draw_model(surface, model, azimuth, altitude, zoom, args.static_light,
                    args.lum_chars, args.color_support);

            // Print surface
            move(0, 0);
            surface_printw(surface);
            refresh();

            if ((args.finite && clock - start >= duration))
                break;

            int key = getch();
            if (key == KEY_RESIZE)
            {
                surface_free(surface);
                surface = create_surface(model, args.surface_width, args.surface_height, args.aspect_ratio, args.stretch);
                if (!surface)
                    return 1;
            }
            else if (key != ERR)
            {
                break;
            }

            tick(&clock, frame_duration);

            t++;
        }

        endwin();
    }

    // Free memory
    surface_free(surface);
    model_free(model);
}
