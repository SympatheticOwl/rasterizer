/**
 * A miminal use of libpng for UIUC's CS418 class.
 *
 * See uselibpng.h for usage examples.
 *
 * Written by Luther Tychonievich by consulting "PNG: The Definitive Guide" by Greg Roelofs <http://www.libpng.org/pub/png/book/>, with a focus on brevity for the common case. Notably, I omitted the recommended setjmp/longjmp error handling entirely.
 */

#ifdef __cplusplus
extern "C" {
#endif

#include <png.h>
#include <stdlib.h>
#include "uselibpng.h"

image_t *load_image(const char *filename) {
  png_structp ps = NULL;
  png_infop pi = NULL;
  image_t *ans = NULL;
  FILE *fp = NULL;
  uint8_t header[8];
  png_byte color = 0, depth = 0;

  fp = fopen(filename, "rb");
  if (!fp) goto fail1;
  fread(header, 1, 8, fp);
  if (png_sig_cmp(header, 0, 8)) goto fail2;
  ps = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
  if (!ps) goto fail2;
  pi = png_create_info_struct(ps);
  if (!pi) goto fail3;

  png_set_sig_bytes(ps, 8);
  png_init_io(ps, fp);
  png_set_keep_unknown_chunks(ps, 1, NULL, 0);
  png_read_info(ps, pi);

  ans = (image_t *)calloc(1, sizeof(image_t));
  if (!ans) goto fail3;
  ans->width = png_get_image_width(ps, pi);
  ans->height = png_get_image_height(ps, pi);

  color = png_get_color_type(ps, pi);
  depth = png_get_bit_depth(ps, pi);

  if (depth == 16) png_set_strip_16(ps);
  if (color == PNG_COLOR_TYPE_PALETTE) png_set_palette_to_rgb(ps);
  if (color == PNG_COLOR_TYPE_GRAY && depth < 8) png_set_expand_gray_1_2_4_to_8(ps);
  if (color == PNG_COLOR_TYPE_GRAY_ALPHA || color == PNG_COLOR_TYPE_GRAY) png_set_gray_to_rgb(ps);
  if (png_get_valid(ps, pi, PNG_INFO_tRNS)) png_set_tRNS_to_alpha(ps);
  if (color == PNG_COLOR_TYPE_RGB || color == PNG_COLOR_TYPE_GRAY || color == PNG_COLOR_TYPE_PALETTE)
    png_set_filler(ps, 0xFF, PNG_FILLER_AFTER); // set missing alpha to opaque

  png_read_update_info(ps, pi);

  ans->rgba = (pixel_t *)malloc(ans->width * ans->height * sizeof(pixel_t));
  if (!ans->rgba) goto fail4;

  for(int passes = png_set_interlace_handling(ps); passes >= 1; passes-=1) {
    for(uint32_t i=0; i<ans->height; i+=1) {
      png_read_row(ps, (png_byte *)&(ans->rgba[ans->width*i]), NULL);
    }
  }

  png_read_end(ps, NULL);
  goto succeed;

  fail4: free(ans);
  succeed: fail3: png_destroy_read_struct(&ps, &pi, NULL);
  fail2: fclose(fp);
  fail1: return ans;
}

void save_image(image_t *img, const char *filename) {
  png_structp ps = NULL;
  png_infop pi = NULL;
  FILE *out = NULL;

  ps = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
  if (!ps) goto fail1;
  pi = png_create_info_struct(ps);
  if (!pi) goto fail2;

  out = fopen(filename, "wb");
  if (!out) goto fail2;
  png_init_io(ps, out);
  //png_set_compression_level(ps, Z_BEST_COMPRESSION);
  png_set_IHDR(ps, pi, img->width, img->height,
    8, // bits per channel
    PNG_COLOR_TYPE_RGB_ALPHA,
    PNG_INTERLACE_NONE,
    PNG_COMPRESSION_TYPE_DEFAULT,
    PNG_FILTER_TYPE_DEFAULT
  );
  png_write_info(ps, pi);
  png_set_packing(ps);

  for(uint32_t i=0; i<img->height; i+=1) {
    png_write_row(ps, (png_byte *)&(img->rgba[img->width*i]));
  }

  png_write_end(ps, NULL);


  fclose(out);
  fail2: png_destroy_write_struct(&ps, &pi);
  fail1: return;
}

image_t *new_image(uint32_t width, uint32_t height) {
  image_t *data = (image_t *)malloc(sizeof(image_t));
  if (!data) return NULL;
  data->width = width;
  data->height = height;
  data->rgba = (pixel_t *)calloc(width*height, sizeof(pixel_t));
  if (!data->rgba) { free(data); return NULL; }
  else return data;
}

void free_image(image_t *img) {
  if (!img) return;
  if (img->rgba) free(img->rgba);
  img->width = 0;
  img->height = 0;
  img->rgba = NULL;
  free(img);
}

#ifdef __cplusplus
}
#endif
