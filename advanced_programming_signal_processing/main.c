#include "imageUtil.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <math.h>

// ---------------------------------------------------------
// 画像を縦横1/2に縮小する関数（透過ピクセル0を考慮）
// ---------------------------------------------------------
Image* createHalfImage(Image* src) {
    Image* dst = createImage(src->width / 2, src->height / 2, src->channel);
    if (!dst) return NULL;

    for (int y = 0; y < dst->height; y++) {
        for (int x = 0; x < dst->width; x++) {
            if (src->channel == 1) {
                int sum = 0, count = 0;
                for (int dy = 0; dy < 2; dy++) {
                    for (int dx = 0; dx < 2; dx++) {
                        int p = src->data[((y * 2 + dy) * src->width + (x * 2 + dx))];
                        if (p != 0) { // 透過考慮[cite: 1]
                            sum += p;
                            count++;
                        }
                    }
                }
                dst->data[y * dst->width + x] = (count == 0) ? 0 : (sum / count);
            } 
            else if (src->channel == 3) {
                int sum_r = 0, sum_g = 0, sum_b = 0, count = 0;
                for (int dy = 0; dy < 2; dy++) {
                    for (int dx = 0; dx < 2; dx++) {
                        int pt = ((y * 2 + dy) * src->width + (x * 2 + dx)) * 3;
                        int r = src->data[pt + 0], g = src->data[pt + 1], b = src->data[pt + 2];
                        if (r != 0 || g != 0 || b != 0) { // いらすとや等の背景透過[cite: 3]
                            sum_r += r; sum_g += g; sum_b += b;
                            count++;
                        }
                    }
                }
                int pt_dst = (y * dst->width + x) * 3;
                if (count == 0) {
                    dst->data[pt_dst + 0] = 0; dst->data[pt_dst + 1] = 0; dst->data[pt_dst + 2] = 0;
                } else {
                    dst->data[pt_dst + 0] = sum_r / count;
                    dst->data[pt_dst + 1] = sum_g / count;
                    dst->data[pt_dst + 2] = sum_b / count;
                }
            }
        }
    }
    return dst;
}

// ---------------------------------------------------------
// ZNCCによる探索範囲指定マッチング (Gray)
// ---------------------------------------------------------
void templateMatchingGrayZNCCArea(Image *src, Image *template, Point *position, double *distance, int start_x, int start_y, int end_x, int end_y, int step)
{
    if (start_x < 0) start_x = 0;
    if (start_y < 0) start_y = 0;
    if (end_x > src->width - template->width) end_x = src->width - template->width;
    if (end_y > src->height - template->height) end_y = src->height - template->height;

    double min_score = 1e9;
    int ret_x = start_x;
    int ret_y = start_y;

    // テンプレートの事前計算 (平均と分散)
    double t_mean = 0.0;
    int valid_pixels = 0;
    for (int j = 0; j < template->height; j += step) {
        for (int i = 0; i < template->width; i += step) {
            int val = template->data[j * template->width + i];
            if (val == 0) continue;
            t_mean += val;
            valid_pixels++;
        }
    }
    if (valid_pixels == 0) { *distance = 1.0; return; }
    t_mean /= valid_pixels;

    double t_var = 0.0;
    for (int j = 0; j < template->height; j += step) {
        for (int i = 0; i < template->width; i += step) {
            int val = template->data[j * template->width + i];
            if (val == 0) continue;
            t_var += (val - t_mean) * (val - t_mean);
        }
    }
    double t_std = sqrt(t_var);

    for (int y = start_y; y <= end_y; y++) {
        for (int x = start_x; x <= end_x; x++) {
            double i_mean = 0.0;
            for (int j = 0; j < template->height; j += step) {
                for (int i = 0; i < template->width; i += step) {
                    if (template->data[j * template->width + i] == 0) continue;
                    i_mean += src->data[(y + j) * src->width + (x + i)];
                }
            }
            i_mean /= valid_pixels;

            double covar = 0.0;
            double i_var = 0.0;
            for (int j = 0; j < template->height; j += step) {
                for (int i = 0; i < template->width; i += step) {
                    int t_val = template->data[j * template->width + i];
                    if (t_val == 0) continue;
                    int s_val = src->data[(y + j) * src->width + (x + i)];
                    covar += (s_val - i_mean) * (t_val - t_mean);
                    i_var += (s_val - i_mean) * (s_val - i_mean);
                }
            }
            
            double i_std = sqrt(i_var);
            double zncc = (t_std > 0 && i_std > 0) ? (covar / (t_std * i_std)) : 0.0;
            double score = 1.0 - zncc; // 完全一致で0になるように変換

            if (score < min_score) {
                min_score = score;
                ret_x = x;
                ret_y = y;
            }
        }
    }
    position->x = ret_x;
    position->y = ret_y;
    *distance = min_score;
}

// ---------------------------------------------------------
// ZNCCによる探索範囲指定マッチング (Color)
// ---------------------------------------------------------
void templateMatchingColorZNCCArea(Image *src, Image *template, Point *position, double *distance, int start_x, int start_y, int end_x, int end_y, int step)
{
    if (start_x < 0) start_x = 0;
    if (start_y < 0) start_y = 0;
    if (end_x > src->width - template->width) end_x = src->width - template->width;
    if (end_y > src->height - template->height) end_y = src->height - template->height;

    double min_score = 1e9;
    int ret_x = start_x;
    int ret_y = start_y;

    double t_mean = 0.0;
    int valid_pixels = 0; // RGB各要素を1ピクセルとしてカウント
    for (int j = 0; j < template->height; j += step) {
        for (int i = 0; i < template->width; i += step) {
            int pt = 3 * (j * template->width + i);
            int r = template->data[pt+0], g = template->data[pt+1], b = template->data[pt+2];
            if (r == 0 && g == 0 && b == 0) continue; // 背景黒を無視[cite: 1, 3]
            t_mean += r + g + b;
            valid_pixels += 3;
        }
    }
    if (valid_pixels == 0) { *distance = 1.0; return; }
    t_mean /= valid_pixels;

    double t_var = 0.0;
    for (int j = 0; j < template->height; j += step) {
        for (int i = 0; i < template->width; i += step) {
            int pt = 3 * (j * template->width + i);
            int r = template->data[pt+0], g = template->data[pt+1], b = template->data[pt+2];
            if (r == 0 && g == 0 && b == 0) continue;
            t_var += (r - t_mean)*(r - t_mean) + (g - t_mean)*(g - t_mean) + (b - t_mean)*(b - t_mean);
        }
    }
    double t_std = sqrt(t_var);

    for (int y = start_y; y <= end_y; y++) {
        for (int x = start_x; x <= end_x; x++) {
            double i_mean = 0.0;
            for (int j = 0; j < template->height; j += step) {
                for (int i = 0; i < template->width; i += step) {
                    int pt2 = 3 * (j * template->width + i);
                    if (template->data[pt2+0] == 0 && template->data[pt2+1] == 0 && template->data[pt2+2] == 0) continue;
                    int pt = 3 * ((y + j) * src->width + (x + i));
                    i_mean += src->data[pt+0] + src->data[pt+1] + src->data[pt+2];
                }
            }
            i_mean /= valid_pixels;

            double covar = 0.0;
            double i_var = 0.0;
            for (int j = 0; j < template->height; j += step) {
                for (int i = 0; i < template->width; i += step) {
                    int pt2 = 3 * (j * template->width + i);
                    if (template->data[pt2+0] == 0 && template->data[pt2+1] == 0 && template->data[pt2+2] == 0) continue;
                    
                    int tr = template->data[pt2+0], tg = template->data[pt2+1], tb = template->data[pt2+2];
                    int pt = 3 * ((y + j) * src->width + (x + i));
                    int sr = src->data[pt+0], sg = src->data[pt+1], sb = src->data[pt+2];
                    
                    covar += (sr - i_mean)*(tr - t_mean) + (sg - i_mean)*(tg - t_mean) + (sb - i_mean)*(tb - t_mean);
                    i_var += (sr - i_mean)*(sr - i_mean) + (sg - i_mean)*(sg - i_mean) + (sb - i_mean)*(sb - i_mean);
                }
            }
            
            double i_std = sqrt(i_var);
            double zncc = (t_std > 0 && i_std > 0) ? (covar / (t_std * i_std)) : 0.0;
            double score = 1.0 - zncc; // 互換性のため 1.0 - ZNCC

            if (score < min_score) {
                min_score = score;
                ret_x = x;
                ret_y = y;
            }
        }
    }
    position->x = ret_x;
    position->y = ret_y;
    *distance = min_score;
}

// ---------------------------------------------------------
// ピラミッド構造を利用した再帰的ZNCCマッチング
// ---------------------------------------------------------
void templateMatchingPyramidGray(Image *src, Image *template, Point *position, double *distance, int levels)
{
    if (levels <= 0 || template->width < 16 || template->height < 16) {
        templateMatchingGrayZNCCArea(src, template, position, distance, 0, 0, src->width, src->height, 1);
        return;
    }

    Image *src_half = createHalfImage(src);
    Image *tpl_half = createHalfImage(template);

    Point coarse_pos;
    double coarse_dist;
    
    // 大局的探索（間引いて高速化）
    templateMatchingPyramidGray(src_half, tpl_half, &coarse_pos, &coarse_dist, levels - 1);

    freeImage(src_half);
    freeImage(tpl_half);

    // ZNCCはピークが少し鈍る場合があるため、探索マージンを少し広め(3)に設定
    int margin = 3;
    int start_x = coarse_pos.x * 2 - margin;
    int start_y = coarse_pos.y * 2 - margin;
    int end_x   = coarse_pos.x * 2 + margin;
    int end_y   = coarse_pos.y * 2 + margin;

    templateMatchingGrayZNCCArea(src, template, position, distance, start_x, start_y, end_x, end_y, 1);
}

void templateMatchingPyramidColor(Image *src, Image *template, Point *position, double *distance, int levels)
{
    if (levels <= 0 || template->width < 16 || template->height < 16) {
        templateMatchingColorZNCCArea(src, template, position, distance, 0, 0, src->width, src->height, 1);
        return;
    }

    Image *src_half = createHalfImage(src);
    Image *tpl_half = createHalfImage(template);

    Point coarse_pos;
    double coarse_dist;
    
    // 大局的探索
    templateMatchingPyramidColor(src_half, tpl_half, &coarse_pos, &coarse_dist, levels - 1);

    freeImage(src_half);
    freeImage(tpl_half);

    int margin = 3; 
    int start_x = coarse_pos.x * 2 - margin;
    int start_y = coarse_pos.y * 2 - margin;
    int end_x   = coarse_pos.x * 2 + margin;
    int end_y   = coarse_pos.y * 2 + margin;

    templateMatchingColorZNCCArea(src, template, position, distance, start_x, start_y, end_x, end_y, 1);
}

// ---------------------------------------------------------
// main関数
// ---------------------------------------------------------
int main(int argc, char **argv)
{
    if (argc < 5)
    {
        fprintf(stderr, "Usage: templateMatching src_image temlate_image rotation threshold option(c,w,p,g)\n");
        return -1;
    }

    char *input_file = argv[1];
    char *template_file = argv[2];
    int rotation = atoi(argv[3]);
    double threshold = atof(argv[4]); // ※ZNCCの場合はスケールが変わるため、run.sh側での閾値調整が必要です

    // printf("rotation -> %d\n", rotation); // 不要な標準出力を抑制してシェルスクリプトを安定化させるのも手です

    char output_name_base[256];
    char output_name_txt[256];
    char output_name_img[256];
    strcpy(output_name_base, "result/");
    strcat(output_name_base, getBaseName(input_file));
    strcpy(output_name_txt, output_name_base);
    strcat(output_name_txt, ".txt");
    strcpy(output_name_img, output_name_base);

    int isWriteImageResult = 0;
    int isPrintResult = 0;
    int isGray = 0;

    if (argc == 6)
    {
        if (strchr(argv[5], 'c') != NULL) clearResult(output_name_txt);
        if (strchr(argv[5], 'w') != NULL) isWriteImageResult = 1;
        if (strchr(argv[5], 'p') != NULL) isPrintResult = 1;
        if (strchr(argv[5], 'g') != NULL) isGray = 1;
    }

    Image *img = readPXM(input_file);
    Image *template = readPXM(template_file);

    Point result;
    double distance = 0.0;
    int pyramid_levels = 2; // ピラミッドの階層

    if (isGray && img->channel == 3)
    {
        Image *img_gray = createImage(img->width, img->height, 1);
        Image *template_gray = createImage(template->width, template->height, 1);
        cvtColorGray(img, img_gray);
        cvtColorGray(template, template_gray);

        templateMatchingPyramidGray(img_gray, template_gray, &result, &distance, pyramid_levels);

        freeImage(img_gray);
        freeImage(template_gray);
    }
    else
    {
        templateMatchingPyramidColor(img, template, &result, &distance, pyramid_levels);
    }

    if (distance < threshold)
    {
        writeResult(output_name_txt, getBaseName(template_file), result, template->width, template->height, rotation, distance);
        if (isPrintResult)
        {
            printf("[Found    ] %s %d %d %d %d %d %f\n", getBaseName(template_file), result.x, result.y, template->width, template->height, rotation, distance);
        }
        if (isWriteImageResult)
        {
            drawRectangle(img, result, template->width, template->height);

            if (img->channel == 3) strcat(output_name_img, ".ppm");
            else if (img->channel == 1) strcat(output_name_img, ".pgm");
            printf("out: %s", output_name_img);
            writePXM(output_name_img, img);
        }
    }
    else
    {
        if (isPrintResult)
        {
            printf("[Not found] %s %d %d %d %d %d %f\n", getBaseName(template_file), result.x, result.y, template->width, template->height, rotation, distance);
        }
    }

    freeImage(img);
    freeImage(template);

    return 0;
}