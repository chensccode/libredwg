/*****************************************************************************/
/*  ts_rewrite.c
 *  从模板文件读取，然后添加新元素生成新文件
 *  参考programs/dwgwrite.c                                                  */
/*****************************************************************************/

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <time.h>

#include "dwg.h"
#include "dwg_api.h"
#include "../jsmn/jsmn.h"

/**
 * 读取模板文件
 * @param filename_in 模板文件
 * @param dwg dwg对象
 * @return
 */
int ts_geojson_dwg_read(const char *filename_in, Dwg_Data *restrict dwg) {

    // 读取模板文件
    int error = dwg_read_file(filename_in, dwg);
    if (error >= DWG_ERR_CRITICAL) {
        printf("模板文件读取失败 0x%x\n", error);
        return error;
    }

//    BITCODE_BL num_objects = dwg.num_objects;
//    printf("num_objecgts %d\n", num_objects);

    // 设置版本
    dwg->header.version = R_2000;
    dwg->header.from_version = dwg->header.version;

    return 1;
}

/**
 * 解析坐标
 *
 * @param geojson 原始geojson字符
 * @param tokens 原始token数组
 * @param start 开始索引
 * @param end 结束的索引
 * @param coordinates 坐标数组
 * @return 坐标点的个数
 */
int ts_geojson_coord(const char *geojson, jsmntok_t *tokens, int start, int end, double *coordinates) {
    char *ptr;
    int i = 0;
    for (int index = start; index < end; index++) {
        const jsmntok_t t = tokens[index];
        int len_key = t.end - t.start;
        char key[len_key];
        memcpy (key, geojson + t.start, len_key);
        key[len_key] = '\0';
        if (t.size == 0) {
            double coord = strtod(key, &ptr);
            coordinates[i] = coord;
            i++;
        }
    }
    return i;
}

/**
 * 获取要素图层名
 *
 * @param geojson 原始geojson字符
 * @param tokens 原始token数组
 * @param index 属性在tokens中的索引
 * @param size 可计算的token数量
 * @param layer_name 图层名
 */
int ts_feature_layer(const char *geojson, jsmntok_t *tokens, int index, int size, char *layer_name) {
    for (int i = 0; i < size; i++) {
        const jsmntok_t t = tokens[index];
        int len_key = t.end - t.start;
        char key[len_key];
        memcpy (key, geojson + t.start, len_key);
        key[len_key] = '\0';
        printf("      key:   %5d(含%d) - %s\n", index, t.size, key);

        // 获取value
        const jsmntok_t tv = tokens[index + 1];
        int len_value = tv.end - tv.start;
        char value[len_value];
        memcpy (value, geojson + tv.start, len_value);
        value[len_value] = '\0';
        printf("      value: %5d(含%d) - %s\n", index + 1, tv.size, value);


        // 判断是否为layer_name
        if (strncmp("__layer_name__", key, len_key) == 0) {
            // 获取value
//            const jsmntok_t tv = tokens[index + 1];
//            int len_value = tv.end - tv.start;
            memcpy (layer_name, geojson + tv.start, len_value);
            layer_name[len_value] = '\0';
            break;
        }
        index += 2;
    }
    return 1;
}

/**
 * 解析属性
 *
 * @param geojson 原始geojson字符
 * @param tokens 原始token数组
 * @param index 属性在tokens中的索引
 * @param size 可计算的token数量
 * @return 状态
 */
int ts_geojson_properties(const char *geojson, jsmntok_t *tokens, int index, int size) {
//    printf("      ************  ts_geojson_properties  ************\n");
    for (int i = 0; i < size; i++) {
        const jsmntok_t t = tokens[index];
        int len_key = t.end - t.start;
        char key[len_key];
        memcpy (key, geojson + t.start, len_key);
        key[len_key] = '\0';
//        printf("      key:   %5d(含%d) - %s\n", index, t.size, key);

        // 获取value
        const jsmntok_t tv = tokens[index + 1];
        int len_value = tv.end - tv.start;
        char value[len_value];
        memcpy (value, geojson + tv.start, len_value);
        value[len_value] = '\0';
//        printf("      value: %5d(含%d) - %s\n", index + 1, tv.size, value);

        index += 2;
//        printf("      -----------------------------------\n");
    }
    return 1;
}

/**
 * 解析几何对象
 *
 * @param geojson 原始geojson字符
 * @param tokens 原始token数组
 * @param index feature数组在token中索引
 * @param size 可计算的token数量
 * @param geom_type 几何类型
 * @param coordinates 坐标
 * @return 坐标点个数
 */
int
ts_geojson_geometry(const char *geojson, jsmntok_t *tokens, int index, int size, char *geom_type, double *coordinates) {
//    printf("      ************  ts_geojson_geometry  ************\n");

    // 坐标点个数
    int coord_count = 0;

    // 遍历geometry
    for (int i = 0; i < size; i++) {
        const jsmntok_t t = tokens[index];
        int len_key = t.end - t.start;
        char key[len_key];
        memcpy (key, geojson + t.start, len_key);
        key[len_key] = '\0';
//        printf("      key:   %5d(含%d) - %s\n", index, t.size, key);

        // 获取value
        const jsmntok_t tv = tokens[index + 1];
        int len_value = tv.end - tv.start;
        char value[len_value];
        memcpy (value, geojson + tv.start, len_value);
        value[len_value] = '\0';
//        printf("      value: %5d(含%d) - %s\n", index + 1, tv.size, value);

        // 解析value, 获取token数量
        jsmn_parser v_parser;
        jsmn_init(&v_parser);
        int v_count = jsmn_parse(&v_parser, value, len_value, NULL, 0);

        // 类型
        if (strncmp("type", key, len_key) == 0) {
            memcpy (geom_type, geojson + tv.start, len_value);
            geom_type[len_value] = '\0';
        } else if (strncmp("coordinates", key, len_key) == 0) {
            int next_start = index + 2;
            int next_end = next_start + v_count;
            coord_count = ts_geojson_coord(geojson, tokens, next_start, next_end, coordinates);
        }

        // 获取下一个feature
        index += (v_count + 1);
//        printf("      -----------------------------------\n");
    }
    return coord_count;
}

/**
 * 解析feature
 *
 * @param dwg_hdr dwg_hdr
 * @param geojson 原始geojson字符
 * @param tokens 原始token数组
 * @param index 要素索引
 * @param size 要素属性的个数
 */
int ts_geojson_feature(Dwg_Object_BLOCK_HEADER *dwg_hdr, const char *geojson, jsmntok_t *tokens, int index, int size) {
    printf("    ************  ts_geojson_feature  ************\n");

    // 要素所在图层
    char layer_name[100] = {0};
    int index_prop = 0;
    int size_prop = 0;

    // 几何类型
    char geom_type[100] = {0};
    double *coordinates;
    int coord_count = 0;

    // 遍历属性
    for (int i = 0; i < size; i++) {
        // 获取要素key
        const jsmntok_t t = tokens[index];
        int len_key = t.end - t.start;
        char key[len_key];
        memcpy (key, geojson + t.start, len_key);
        key[len_key] = '\0';
//        printf("    key:   %5d(含%d) - %s\n", index, t.size, key);

        // 获取value
        const jsmntok_t tv = tokens[index + 1];
        int len_value = tv.end - tv.start;
        char value[len_value];
        memcpy (value, geojson + tv.start, len_value);
        value[len_value] = '\0';
//        printf("    value: %5d(含%d) - %s\n", index + 1, tv.size, value);

        // 解析value, 获取token数量
        jsmn_parser v_parser;
        jsmn_init(&v_parser);
        int v_count = jsmn_parse(&v_parser, value, len_value, NULL, 0);

        // 判断properties geometry
        int next_index = index + 2;
        if (strcmp("properties", key) == 0) {
//            ts_geojson_properties(geojson, tokens, next_index, tv.size);
            ts_feature_layer(geojson, tokens, next_index, tv.size, layer_name);
            index_prop = next_index;
            size_prop = tv.size;
        } else if (strcmp("geometry", key) == 0) {
            coordinates = (double *) malloc(v_count * sizeof(double));
            memset(coordinates, 0, sizeof(double) * v_count);
            coord_count = ts_geojson_geometry(geojson, tokens, next_index, tv.size, geom_type, coordinates);
        }

        // 获取下一个属性key
        index += (v_count + 1);
//        printf("    -----------------------------------\n");
    }

    printf("    图层名: %s, 类型: %s\n", layer_name, geom_type);

    // 写入到图层
    // 添加点、线、文字
    if (strcmp("Point", geom_type) == 0) {
        for (int i = 0; i < coord_count / 2; i++) {
            dwg_point_3d pt1 = {coordinates[i], coordinates[i + 1]};
            dwg_add_CIRCLE(dwg_hdr, &pt1, 2);
        }

    } else if (strcmp("LineString", geom_type) == 0) {
        // 单线段
//        for (int i = 0; i < coord_count / 4; i++) {
//            dwg_point_3d pt1 = {coordinates[i], coordinates[i + 1]};
//            dwg_point_3d pt2 = {coordinates[i+2], coordinates[i + 3]};
//            dwg_add_LINE(dwg_hdr, &pt1, &pt2);
//        }
        // 多线段
        int num_pts = coord_count / 2;
        dwg_point_3d pts[num_pts];
        for (int i = 0; i < num_pts; i++) {
            pts[i].x = coordinates[i * 2];
            pts[i].y = coordinates[i * 2 + 1];
        }
        dwg_add_POLYLINE_3D(dwg_hdr, num_pts, pts);
    }

    // 释放坐标
    if (coordinates) {
        free(coordinates);
    }

    return 1;
}

/**
 * 解析feature数组
 *
 * @param dwg_hdr dwg_hdr
 * @param geojson 原始geojson字符
 * @param tokens 原始token数组
 * @param index feature数组在token中索引
 * @param size feature数量
 */
int
ts_geojson_feature_arr(Dwg_Object_BLOCK_HEADER *dwg_hdr, const char *geojson, jsmntok_t *tokens, int index, int size) {
    printf("  ************  ts_geojson_feature_arr  ************\n");

    // 遍历(一级key, 并查找features)
    for (int i = 0; i < size; i++) {
        // 获取key(实际为一个具体feature字符串)
        const jsmntok_t t = tokens[index];
        int len_key = t.end - t.start;
        char key[len_key];
        memcpy (key, geojson + t.start, len_key);
        key[len_key] = '\0';
//        printf("  key:   %5d(含%d) - %s\n", index, t.size, key);

        // 解析当前feature含的token数量
        jsmn_parser v_parser;
        jsmn_init(&v_parser);
        int v_count = jsmn_parse(&v_parser, key, len_key, NULL, 0);

        // 解析feature
        int index_start = index + 1;
        ts_geojson_feature(dwg_hdr, geojson, tokens, index_start, t.size);

        // 获取下一个feature
        index += v_count;
        printf("  -----------------------------------\n");
    }
    return 1;
}

/**
 * geojson导出为dwg
 * @param filename_in 模板文件
 * @param filename_out 输出文件
 * @param geojson geojson
 * @return 1-success, 0-error
 */
int ts_geojson_to_dwg(const char *filename_in, const char *filename_out, const char *geojson) {
    // 读取dwg模板文件
    int error;
    Dwg_Data dwg;
    Dwg_Object_BLOCK_HEADER *dwg_hdr;
    memset (&dwg, 0, sizeof(Dwg_Data));
    error = ts_geojson_dwg_read(filename_in, &dwg);
    Dwg_Object *mspace = dwg_model_space_object(&dwg);
    dwg_hdr = mspace->tio.object->tio.BLOCK_HEADER;

    // test
    Dwg_Object_LAYER *layer = dwg_add_LAYER(&dwg, (const BITCODE_T) "测试");


    // 解析参考 in_json.c
    jsmn_parser parser;
    jsmntok_t *tokens;
    int num_tokens;

    // 解析json
    jsmn_init(&parser);
    num_tokens = jsmn_parse(&parser, geojson, strlen(geojson), NULL, 0);
//    printf("tokens 数量: %ld\n", tokens.num_tokens);
    tokens = (jsmntok_t *) calloc(num_tokens + 1024, sizeof(jsmntok_t));
    if (!tokens) {
        printf("json错误 \n");
        return 0;
    }

    // 获取所有tokens
    jsmn_init(&parser);
    jsmn_parse(&parser, geojson, strlen(geojson), tokens, (unsigned int) num_tokens);

    // 遍历(一级key, 并查找features)
    for (int i = 1; i < num_tokens; i++) {
        // 获取key
        const jsmntok_t t = tokens[i];
        int len_key = t.end - t.start;
        char key[len_key];
        memcpy (key, geojson + t.start, len_key);
        key[len_key] = '\0';
//        printf("key:   %5d(含%d) - %s\n", i, t.size, key);

        // 获取value
        const jsmntok_t tv = tokens[i + 1];
        int len_value = tv.end - tv.start;
        char value[len_value];
        memcpy (value, geojson + tv.start, len_value);
        value[len_value] = '\0';
//        printf("value: %5d(含%d) - %s\n", i + 1, tv.size, value);

        // 解析value, 获取token数量
        jsmn_parser v_parser;
        jsmn_init(&v_parser);
        int v_count = jsmn_parse(&v_parser, value, len_value, NULL, 0);

        // 判断是否为features
        if (strncmp("features", key, len_key) == 0) {
            int index_start = i + 2;
            ts_geojson_feature_arr(dwg_hdr, geojson, tokens, index_start, tv.size);
        }

        // 越过key-value, 获取下一个一级key
        i += v_count;
//        printf("-----------------------------------\n");
    }

    // 输出到文件
    error = dwg_write_file(filename_out, &dwg);
    if (error >= DWG_ERR_CRITICAL) {
        printf("输出文件错误 0x%x\n", error);
    } else {
        printf("输出文件ok %s \n", filename_out);
    }
    dwg_free(&dwg);

    return 1;
}

/**
 * 删除左侧空白字符
 *
 * @param str str
 */
void ts_ltrim(char *str) {
    if (str == NULL || *str == '\0') {
        return;
    }

    int len = 0;
    char *p = str;
    while (*p != '\0' && isspace(*p)) {
        ++p;
        ++len;
    }
    memmove(str, p, strlen(str) - len + 1);
}

/**
 * 删除右侧空白字符
 *
 * @param str str
 */
void ts_rtrim(char *str) {
    if (str == NULL || *str == '\0') {
        return;
    }
    unsigned long len = strlen(str);
    char *p = str + len - 1;
    while (p >= str && isspace(*p)) {
        *p = '\0';
        --p;
    }
}

/**
 * 删除左右两侧空白字符
 *
 * @param str str
 */
void ts_trim(char *s) {
    ts_ltrim(s);
    ts_rtrim(s);
}

/**
 * 读取geojson文本文件
 * @param filename  geojson文件名
 * @param geojson  geojson文本
 * @return 文本字符串, 用完后需释放内存
 */
char *ts_read_geojson_txt(const char *filename) {

    FILE *file;
    if ((file = fopen(filename, "r")) == NULL) {
        perror("fail to read");
        return 0;
    }

    // 获取字符数
    fseek(file, 0, SEEK_END);
    long size = ftell(file) + 1;

    // 读取文件
    char *geojson;
    geojson = (char *) malloc(size);
    memset(geojson, 0, size);

    // 读取全部内容, 不去空格
//    fread(geojson, sizeof(char), size, file);

    // 遍历行读取, 去除前后空格
    rewind(file);
    char line[size];
    while (!feof(file)) {
        memset(line, 0, size);
        fgets(line, (int) size, file);

        // 去除前后空格、换行符, 拷贝到geojson
        ts_trim(line);
        strncat(geojson, line, strlen(line));
    }
    fclose(file);
    return geojson;
}


/**
 * 测试输出文件
 * @return
 */
int ts_test_out() {

    const char *filename_in = "../examples-tssu/r15.dwg";

    // 根据日期时间生成输出文件名
    char filename_out[500];
    time_t timer = time(NULL);
    sprintf(filename_out, "/Users/chensc/Downloads/dwg-%ld.dwg", timer);
//    const char *filename_out = "/Users/chensc/Downloads/r15-re.dwg";

    int error;
    Dwg_Data dwg;
    Dwg_Object *mspace;
    Dwg_Object_BLOCK_HEADER *hdr;

    // 读取文件
    error = dwg_read_file(filename_in, &dwg);
    if (error >= DWG_ERR_CRITICAL) {
        printf("模板文件读取失败 0x%x\n", error);
    }

    BITCODE_BL num_objects = dwg.num_objects;
    printf("num_objecgts %d\n", num_objects);

    // 设置版本
    dwg.header.version = R_2000;
    dwg.header.from_version = dwg.header.version;

    // 绘图空间
    mspace = dwg_model_space_object(&dwg);
    hdr = mspace->tio.object->tio.BLOCK_HEADER;

    // 添加点、线、文字
    dwg_point_3d pt1 = {1.5, 1.5};
    dwg_point_3d pt2 = {2.5, 22.5};
    dwg_add_LINE(hdr, &pt1, &pt2);
    dwg_add_TEXT(hdr, "OKOK-TEXT", &pt2, 1);
    dwg_add_CIRCLE(hdr, &pt1, 2);

    // 输出到文件
    error = dwg_write_file(filename_out, &dwg);
    if (error >= DWG_ERR_CRITICAL) {
        printf("输出文件错误 0x%x\n", error);
    } else {
        printf("输出文件ok\n");
    }
    dwg_free(&dwg);
    return 1;
}

int main(int argc, char *argv[]) {
    const char *filename_in = "../examples-tssu/r15.dwg";

    // 根据日期时间生成输出文件名
    char filename_out[500];
    time_t timer = time(NULL);
    sprintf(filename_out, "/Users/chensc/Downloads/dwg-%ld.dwg", timer);
//    const char *filename_out = "/Users/chensc/Downloads/r15-re.dwg";

    // 1. geojson变量
//    const char *geojson = "{\"name\":[\"type \",\"properties\",\"features\"],\"features\":[{\"type\":\"Feature\",\"properties\":{\"Color\":256,\"__layer_name__\":\"1FB\",\"name\":\"wang1\"},\"geometry\":{\"type2\":\"2222\",\"type\":\"LineString\",\"coordinates\":[[1691.838433,1578.622315],[2508.963712,1021.777627]]}},{\"type\":\"Feature\",\"id\":\"1FC\",\"properties\":{\"Color\":256,\"EntityHandle\":\"1FC\",\"name\":\"zhang22\"},\"geometry\":{\"type\":\"LineString\",\"coordinates\":[[2508.963712,1021.777627],[2625.695894,1543.637306]]}},{\"type\":\"Feature\",\"id\":\"1FC\",\"properties\":{\"Color\":256,\"EntityHandle\":\"1FC\",\"__layer_name__\":\"zhang22\"},\"geometry\":{\"type\":\"LineString\",\"coordinates\":[[2508.963712,1021.777627],[2625.695894,1543.637306]]}}],\"type\":\"FeatureCollection\"}";

    // 2. 从文件读取geojson
    const char *geojson_filename = "../examples-data/d2010.geojson";
    char *geojson = ts_read_geojson_txt(geojson_filename);

    // 输出到文件
    ts_geojson_to_dwg(filename_in, filename_out, geojson);

    free(geojson);

    return 0;
}
