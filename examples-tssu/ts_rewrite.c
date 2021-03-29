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
    tokens.tokens = (jsmntok_t *) calloc(tokens.num_tokens + 1024, sizeof(jsmntok_t));
    if (!tokens.tokens) {
        printf("json错误 \n");
        return 0;
    }

    // reset pos to 0
    jsmn_init(&parser);
    jsmn_parse(&parser, geojson, json_len, tokens.tokens, (unsigned int) tokens.num_tokens);

    printf("tokens 数量: %ld\n", tokens.num_tokens);

    // 遍历
    for (tokens.index = 1; tokens.index < (unsigned int) tokens.num_tokens; tokens.index++) {
        // 获取key
        const jsmntok_t t = tokens.tokens[tokens.index];
        int len_key = t.end - t.start;
        char key[1000];
        memcpy (key, geojson + t.start, len_key);
        key[len_key] = '\0';

        printf("%2d- %2d - %s\n", tokens.index, t.size, key);
//        printf("%d\n", t.size);

        // 获取value
        const jsmntok_t tv = tokens.tokens[tokens.index + 1];
        int len_value = tv.end - tv.start;
        char value[1000];
        memcpy (value, geojson + tv.start, len_value);
        value[len_value] = '\0';
        printf("%2d- %2d - %s\n", tokens.index + 1, tv.size, value);
        tokens.index++;

//        if (strncmp(key, "features", len_key) == 0) {
//            const jsmntok_t tv = tokens.tokens[tokens.index + 1];
//            for (int j = 0; j < tv.size; j++) {
//                const jsmntok_t tk = tokens.tokens[tokens.index + tv.size + 1 + j];
//                int len_key2 = tk.end - tk.start;
//                char key2[1000];
//                memcpy (key2, geojson + tk.start, len_key2);
//                key2[len_key2] = '\0';
//                printf("---- %s\n", key2);
//                tokens.index++;
//            }
//            tokens.index = tv.size + 1;
//        }
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
        printf("输出文件ok 0x%x\n");
    }
    return 1;
}

int main(int argc, char *argv[]) {
    const char *filename_in = "../examples-tssu/r15.dwg";
    const char *filename_out = "../examples-tssu/r15-re.dwg";
    const char *geojson = "{\"name\":[11,22,33],\"type\":\"FeatureCollection\",\"features\":[{\"type\":\"Feature\",\"properties\":{\"Color\":256,\"EntityHandle\":\"1FB\"},\"geometry\":{\"type\":\"LineString\",\"coordinates\":[[1691.838433,1578.622315],[2508.963712,1021.777627]]}},{\"type\":\"Feature\",\"id\":\"1FC\",\"properties\":{\"Color\":256,\"EntityHandle\":\"1FC\"},\"geometry\":{\"type\":\"LineString\",\"coordinates\":[[2508.963712,1021.777627],[2625.695894,1543.637306]]}}]}";

    ts_geojson_to_dwg(filename_in, filename_out, geojson);
    return 0;
}
