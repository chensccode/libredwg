/*****************************************************************************/
/*  ts_rewrite.c
 *  从模板文件读取，然后添加新元素生成新文件
 *  参考programs/dwgwrite.c                                                  */
/*****************************************************************************/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "dwg.h"
#include "dwg_api.h"
#include "bits.h"
#include "../jsmn/jsmn.h"

typedef struct jsmntokens {
    unsigned int index;
    jsmntok_t *tokens;
    long num_tokens;
} jsmntokens_t;


/**
 * 解析属性
 *
 * @param geojson 原始geojson字符
 * @param tokens 原始token数组
 * @param index 属性在tokens中的索引
 * @param size 可计算的token数量
 */
int ts_geojson_properties(const char *geojson, jsmntok_t *tokens, int index, int size) {
    printf("      ************  ts_geojson_properties  ************\n");

    // 遍历(一级key, 并查找features)
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

        index += 2;
        printf("      -----------------------------------\n");
    }
    return 1;
}

/**
 * 解析几何对象
 *
 * @param geojson 原始geojson字符
 * @param tokens 原始token数组
 * @param start feature数组在token中索引
 * @param count 可计算的token数量
 */
int ts_geojson_geometry(const char *geojson, jsmntok_t *tokens, unsigned int start, unsigned int end) {
    printf("      ************  ts_geojson_geometry  ************\n");

    // 遍历(一级key, 并查找features)
    for (unsigned int i = start; i < end; i++) {
        // 获取key(实际为一个具体feature字符串)
        const jsmntok_t t = tokens[i];
        int len_key = t.end - t.start;
        char key[len_key];
        memcpy (key, geojson + t.start, len_key);
        key[len_key] = '\0';
        printf("      key:   %5d(含%d) - %s\n", i, t.size, key);

        // 获取value
        const jsmntok_t tv = tokens[i + 1];
        int len_value = tv.end - tv.start;
        char value[len_value];
        memcpy (value, geojson + tv.start, len_value);
        value[len_value] = '\0';
//        printf("value: %5d(含%d) - %s\n", tokens.index + 1, tv.size, value);

        // 解析value, 获取token数量
        jsmn_parser v_parser;
        jsmn_init(&v_parser);
        long v_count = jsmn_parse(&v_parser, value, len_value, NULL, 0);

        // 判断是否为features
        if (strncmp("coordinates", key, len_key) == 0) {
        }

        // 获取下一个feature
        i += (v_count - 1);
        printf("      -----------------------------------\n");
    }
    return 1;
}

/**
 * 解析feature
 *
 * @param geojson 原始geojson字符
 * @param tokens 原始token数组
 * @param index 要素索引
 * @param size 要素属性的个数
 */
int ts_geojson_feature(const char *geojson, jsmntok_t *tokens, int index, int size) {
    printf("    ************  ts_geojson_feature  ************\n");

    // 遍历属性
    for (int i = 0; i < size; i++) {
        // 获取key(实际为一个具体feature字符串)
        const jsmntok_t t = tokens[index];
        int len_key = t.end - t.start;
        char key[len_key];
        memcpy (key, geojson + t.start, len_key);
        key[len_key] = '\0';
        printf("    key:   %5d(含%d) - %s\n", index, t.size, key);

        // 获取value
        const jsmntok_t tv = tokens[index + 1];
        int len_value = tv.end - tv.start;
        char value[len_value];
        memcpy (value, geojson + tv.start, len_value);
        value[len_value] = '\0';
        printf("    value: %5d(含%d) - %s\n", index + 1, tv.size, value);

        // 解析value, 获取token数量
        jsmn_parser v_parser;
        jsmn_init(&v_parser);
        long v_count = jsmn_parse(&v_parser, value, len_value, NULL, 0);

        // 判断properties geometry
        int next_index = index + 2;
        if (strncmp("properties", key, len_key) == 0) {
            ts_geojson_properties(geojson, tokens, next_index, tv.size);
        } else if (strncmp("geometry", key, len_key) == 0) {
//            ts_geojson_geometry(geojson, tokens, next_start, tv.size);
        }

        // 获取下一个属性key
        index += ((int) v_count + 1);
        printf("    -----------------------------------\n");
    }
    return 1;
}

/**
 * 解析feature数组
 *
 * @param geojson 原始geojson字符
 * @param tokens 原始token数组
 * @param index feature数组在token中索引
 * @param size feature数量
 */
int ts_geojson_feature_arr(const char *geojson, jsmntok_t *tokens, int index, int size) {
    printf("  ************  ts_geojson_feature_arr  ************\n");

    // 遍历(一级key, 并查找features)
    for (int i = 0; i < size; i++) {
        // 获取key(实际为一个具体feature字符串)
        const jsmntok_t t = tokens[index];
        int len_key = t.end - t.start;
        char key[len_key];
        memcpy (key, geojson + t.start, len_key);
        key[len_key] = '\0';
        printf("  key:   %5d(含%d) - %s\n", index, t.size, key);

        // 解析当前feature含的token数量
        jsmn_parser v_parser;
        jsmn_init(&v_parser);
        long v_count = jsmn_parse(&v_parser, key, len_key, NULL, 0);

        // 解析feature
        ts_geojson_feature(geojson, tokens, index + 1, t.size);

        // 获取下一个feature
        index += (int) v_count;
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
    const size_t json_len = strlen(geojson);

    // 解析参考 in_json.c
    jsmn_parser parser;
    jsmntokens_t tokens;

    // 解析json
    jsmn_init(&parser);
    tokens.num_tokens = jsmn_parse(&parser, geojson, json_len, NULL, 0);
//    printf("tokens 数量: %ld\n", tokens.num_tokens);
    tokens.tokens = (jsmntok_t *) calloc(tokens.num_tokens + 1024, sizeof(jsmntok_t));
    if (!tokens.tokens) {
        printf("json错误 \n");
        return 0;
    }

    // reset pos to 0
    jsmn_init(&parser);
    jsmn_parse(&parser, geojson, json_len, tokens.tokens, (unsigned int) tokens.num_tokens);

    // 遍历(一级key, 并查找features)
    for (tokens.index = 1; tokens.index < (unsigned int) tokens.num_tokens; tokens.index++) {

        // 获取key
        const jsmntok_t t = tokens.tokens[tokens.index];
        int len_key = t.end - t.start;
        char key[len_key];
        memcpy (key, geojson + t.start, len_key);
        key[len_key] = '\0';
        printf("key:   %5d(含%d) - %s\n", tokens.index, t.size, key);

        // 获取value
        const jsmntok_t tv = tokens.tokens[tokens.index + 1];
        int len_value = tv.end - tv.start;
        char value[len_value];
        memcpy (value, geojson + tv.start, len_value);
        value[len_value] = '\0';
        printf("value: %5d(含%d) - %s\n", tokens.index + 1, tv.size, value);

        // 解析value, 获取token数量
        jsmn_parser v_parser;
        jsmn_init(&v_parser);
        long v_count = jsmn_parse(&v_parser, value, len_value, NULL, 0);

        // 判断是否为features
        if (strncmp("features", key, len_key) == 0) {
            int index_start = (int) tokens.index + 2;
            ts_geojson_feature_arr(geojson, tokens.tokens, index_start, tv.size);
        }

        // 越过key-value, 获取下一个一级key
        tokens.index += v_count;
        printf("-----------------------------------\n");
    }
    return 1;
}

/**
 * 测试输出文件
 * @return
 */
int ts_test_out() {
    const char *filename_in = "../examples-tssu/r15.dwg";
    const char *filename_out = "../examples-tssu/r15-re.dwg";

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
    return 1;
}

int main(int argc, char *argv[]) {
    const char *filename_in = "../examples-tssu/r15.dwg";
    const char *filename_out = "../examples-tssu/r15-re.dwg";
    const char *geojson = "{\"name\":[\"type \",\"properties\",\"features\"],\"features\":[{\"type\":\"Feature\",\"properties\":{\"Color\":256,\"EntityHandle\":\"1FB\",\"name\":\"wang1\"},\"geometry\":{\"type\":\"LineString\",\"coordinates\":[[1691.838433,1578.622315],[2508.963712,1021.777627]]}},{\"type\":\"Feature\",\"id\":\"1FC\",\"properties\":{\"Color\":256,\"EntityHandle\":\"1FC\",\"name\":\"zhang22\"},\"geometry\":{\"type\":\"LineString\",\"coordinates\":[[2508.963712,1021.777627],[2625.695894,1543.637306]]}}],\"type\":\"FeatureCollection\"}";

    ts_geojson_to_dwg(filename_in, filename_out, geojson);
    return 0;
}
