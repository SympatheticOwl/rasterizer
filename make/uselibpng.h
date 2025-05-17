/**
 * A miminal use of libpng for UIUC's CS418 class.
 *
 * See uselibpng.c for implementation notes
 */


#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

/**
 * RGBA pixel with several ways of looking at it:
 *
 * pixel.p[0] is the first (red) channel of the pixel
 * pixel.red is the red (first) channel of the pixel
 * pixel.r is the red (first) channel of the pixel
 */
typedef union {
  uint8_t p[4];
  struct { uint8_t r, g, b, a; };
  struct { uint8_t red, green, blue, alpha; };
} pixel_t;

/**
 * A minimal struct for storing PNG data. C/C++ has poor support for 2D arrays,
 * so use the provided image_xy macro (C) or the Image wrapper class (C++) for access.
 */
typedef struct {
  uint32_t height, width;
  pixel_t *rgba;
} image_t;



/**
 * Load a PNG from the given file.
 *
 * ~~~~
 * image_t *img = load_image("existing_image.png");
 * if (img == NULL) { fprintf(stderr, "%s was not a PNG file", "existing_image.png"); }
 * for(int y = 0; y < img->height; y += 1) {
 *   for(int x = 0; x < img->width; x += 1) {
 *     pixel_t pixel = pixel_xy(img, x, y);
 *   }
 * }
 * ~~~~
 */
image_t *load_image(const char *filename);

/**
 * Save a PNG to the given file.
 *
 * ~~~~
 * image_t *img = ...
 * save_image(img, "new_image.png")
 * ~~~~
 */
void save_image(image_t *img, const char *filename);

/**
 * Allocate an image with the given width and height.
 *
 * ~~~~
 * image_t *img = new_image(1280, 720);
 * for(int y = 0; y < img->height; y += 1) {
 *   for(int x = 0; x < img->width; x += 1) {
 *     pixel_xy(img, x, y).red = x & 0xFF;
 *     pixel_xy(img, x, y).g = y & 0xFF;
 *     pixel_xy(img, x, y).p[2] = ((x+y)>>8) & 0xFF;
 *     pixel_xy(img, x, y).alpha = (x^y)&1 ? 0xFF : 0x00;
 *   }
 * }
 * save_image(img, "demo_image.png");
 * ~~~~
 */
image_t *new_image(uint32_t width, uint32_t height);

/**
 * Deallocate memory used to store the image
 */
void free_image(image_t *img);


#ifdef __cplusplus
} // end extern "C"
#endif



#ifdef __cplusplus
/**
 * A class wrapper around the image_t structure and its functions
 *
 * ~~~~
 * Image img = Image("image_to_read.png");
 *
 * for(uint32_t y=0; y<img.height(); y+=1) {
 *   for(uint32_t x=0; x<img.width(); x+=1) {
 *     img[y][x].red ^= img[y][x].alpha;
 *     img[y][x].g ^= 0xaa;
 *     img[y][x].blue = 0xFF - img[y][x].blue;
 *     img[y][x].p[3] = 0xFF;
 *   }
 * }
 *
 * img.save("image_to_write.png");
 * ~~~~
 */
class Image {
  public:
    /// Create a blank (transparent black) image of the given size
    Image(uint32_t width, uint32_t height) : data(new_image(width, height)) {}
    /**
     * Create an image by reading a PNG file
     * if you have a std::string, use `Image(string.c_str())`
     */
    Image(const char *filename) : data(load_image(filename)) { }

    /// destructor
    ~Image() { free_image(data); }

    /// array access to image. Use as `img[y][x].red` or the like. Note the y-first order!
    pixel_t* operator[](uint32_t y) { return &(data->rgba[data->width * y]); }

    /// store the image in a PNG file
    void save(const char *filename) const {
      save_image(data, filename);
    }

    /// get the width of the image
    uint32_t width() const { return data->width; }

    /// get the height of the image
    uint32_t height() const { return data->height; }
  private:
    image_t* data;
};
#else
/**
 * A helper macro for accessing the image.
 * pixel_xy(img, x, y).red = 255;
 * opacity = pixel_xy(img, x, y).alpha;
 */
#define pixel_xy(img,x,y) (img)->rgba[(x) + (y)*(img)->width]
#endif

