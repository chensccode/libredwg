/*****************************************************************************/
/*  gesjson_std.c
 *  输出为geojson字符串                                                        */
/*  字符串操作使用标准库方式，区别sds库                                           */
/*  ts_表示清苏开头的自定函数                                                   */
/*****************************************************************************/
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "dwg.h"
#include "dwg_api.h"
#include "bits.h"

/**
 * 扩展数据xdata转json
 * 目前只处理【单点、单线】数据, 数据保留10位小数, 为了兼容经纬度和投影坐标
 * @param obj dwg对象
 * @return 图形json字符串, 默认最长1000个字符串, 需优化修改...
 */
char *ts_object_json_geometry(Dwg_Object *obj) {
    char *json_geometry = (char *) malloc(1000);
    memset(json_geometry, 0, 1000);

    Dwg_Object_Entity *entity = obj->tio.entity;
    if (obj->type == DWG_TYPE_LINE) {
        // 单线
        char str[200] = {0};
        Dwg_Entity_LINE *line = entity->tio.LINE;
        sprintf(str, "[[%.10lf,%.10lf],[%.10lf,%.10lf]]",
                line->start.x, line->start.y, line->end.x, line->end.y);
        strcpy(json_geometry, "\"geometry\":{\"type\":\"LineString\",\"coordinates\":");
        strcat(json_geometry, str);
        strcat(json_geometry, "}");
    } else if (obj->type == DWG_TYPE_LWPOLYLINE) {
        // 多线
        int error1, error2;
        Dwg_Entity_LWPOLYLINE *pline = entity->tio.LWPOLYLINE;
        BITCODE_RL numpts = dwg_ent_lwpline_get_numpoints(pline, &error1);
        dwg_point_2d *pts = dwg_ent_lwpline_get_points(pline, &error2);
        if (numpts && !error1 && !error2) {
            strcpy(json_geometry, "\"geometry\":{\"type\":\"LineString\",\"coordinates\":[");
            for (BITCODE_RL j = 0; j < numpts; j++) {
                char str[100] = {0};
                sprintf(str, "[%.10lf,%.10lf],", pts[j].x, pts[j].y);
                strcat(json_geometry, str);
            }
            json_geometry[strlen(json_geometry) - 1] = 0;
            strcat(json_geometry, "]}");
        }
    } else if (obj->type == DWG_TYPE_INSERT) {
        // 单块
        Dwg_Entity_INSERT *insert = entity->tio.INSERT;
        BITCODE_3DPOINT point = insert->ins_pt;
        char str[100] = {0};
        sprintf(str, "[%.10lf,%.10lf]", point.x, point.y);
        strcat(json_geometry, "\"geometry\":{\"type\":\"Point\",\"coordinates\":");
        strcat(json_geometry, str);
        strcat(json_geometry, "}");
    }

    return json_geometry;
}

/**
 * 判断是否在块列表内
 * @param obj obj
 * @param block_count 块的数量
 * @param block_names 块的名称
 * @return 是否. 如果块列表为0, 直接返回true
 */
bool ts_check_block(Dwg_Object *obj, int block_count, char *block_names[]) {
    // 如果块列表为0, 直接返回true
    if (block_count == 0) {
        return true;
    }

    bool flag = false;
    int error;
    Dwg_Entity_INSERT *insert = obj->tio.entity->tio.INSERT;
    Dwg_Object *hdr = dwg_ref_get_object(insert->block_header, &error);
    if (!error && hdr && hdr->type == DWG_TYPE_BLOCK_HEADER) {
        Dwg_Object_BLOCK_HEADER *_hdr = hdr->tio.object->tio.BLOCK_HEADER;
        char *block_name = bit_convert_TU((BITCODE_TU) _hdr->name);
        // 循环判断
        for (int i = 0; i < block_count; i++) {
            if (strcmp(block_name, block_names[i]) == 0) {
                flag = true;
                break;
            }
        }
        free(block_name);
    }
    return flag;
}

/**
 * 获取扩展数据-xdata转json
 * @param obj dwg对象
 * @param layer_name 当前对象所在图层名
 * @return 属性json字符串
 */
char *ts_object_json_eed(Dwg_Object *obj, const char *layer_name) {
    // 默认大小1000，每次并遍历属性时计算大小重新分配
    size_t size_prop = 1000;
    char *json_eed = (char *) malloc(size_prop);
    memset(json_eed, 0, size_prop);

    strcpy(json_eed, "\"properties\":{\"cad_layer\":\"");
    strcat(json_eed, layer_name);
    strcat(json_eed, "\",");

    Dwg_Object_Object *obj_obj = obj->tio.object;
    int num_eed = obj_obj->num_eed;

    // 无扩展属性时, 删除尾部逗号(,)并追加闭合花括号(})
    if (num_eed == 0) {
        json_eed[strlen(json_eed) - 1] = 0;
        strcat(json_eed, "}");
        return json_eed;
    }

    // 定义长度500, 字段100, 属性400
    int len_key = 100;
    int len_value = 400;
    int len_prop = len_key + len_value;

    // 遍历查找
    for (unsigned i = 0; i < obj_obj->num_eed; i++) {
        Dwg_Eed eed = obj_obj->eed[i];

        // 当前属性
        char *prop = (char *) malloc(len_prop);
        memset(prop, 0, len_prop);

        // 读取1: 属性名
        // 是否包括key, 判断是否为属性的标记
        bool key_flag = false;
        if (eed.size) {
            Dwg_Object *appid = dwg_resolve_handle(obj_obj->dwg, eed.handle.value);
            if (appid && appid->fixedtype == DWG_TYPE_APPID) {
                char *name = appid->tio.object->tio.APPID->name;

                // todo 是否转，需要再确认, 中文有乱码
//                if (IS_FROM_TU_DWG (obj->parent))
                char *name_utf8 = bit_convert_TU((BITCODE_TU) name);
                if (strlen(name_utf8) > 0) {
                    key_flag = true;
                    strcat(prop, "\"");
                    strncat(prop, name_utf8, len_key);
                    strcat(prop, "\":");
                }
                free(name_utf8);
            }
        }
        if (!key_flag) {
            free(prop);
            continue;
        }

        // 读取2: 属性值
        if (eed.data) {
            const Dwg_Eed_Data *data = eed.data;
//            if (IS_FROM_TU_DWG (obj->parent)) {
                char *value_utf8 = bit_convert_TU((BITCODE_TU) data->u.eed_0.string);
                strcat(prop, "\"");
                strncat(prop, value_utf8, len_value);
                strcat(prop, "\",");
                free(value_utf8);
//            } else {
//                strcat(prop, "\"");
//                strncat(prop, data->u.eed_0.string, len_value);
//                strcat(prop, "\",");
//            }
        } else {
            strcat(prop, "\"\",");
        }

        // 判断是否需要重新分配
        size_t len_temp = strlen(json_eed) + strlen(prop) + 20;
        if (size_prop < len_temp) {
            size_prop += len_temp;
            json_eed = (char *) realloc(json_eed, size_prop);
        }
        strcat(json_eed, prop);
        free(prop);
    }

    // 删除尾部逗号(,)并追加闭合花括号(})
    json_eed[strlen(json_eed) - 1] = 0;
    strcat(json_eed, "}");

    return json_eed;
}

/**
 * 数组元素格式
 * @param str 字符串
 * @return 拆分后元素数量, 为空字符串时返回0
 */
int ts_element_count(const char *content_str) {
    if (strlen(content_str) == 0) {
        return 0;
    }
    // 分隔字符, 此处为字节
    char c = '|';
    int count = 0;
    while (*content_str) {
        if (*content_str == c) {
            count++;
        }
        content_str++;
    }
    return count + 1;
}

/**
 * 元素拆分
 * @param elements_str 元素字符串
 * @param elements     返回的字符串数组, 需在外部初始化。在内部分配内存并初始化的时候无法传到外面???
 */
void ts_element_sep(const char *elements_str, char *elements[]) {
    // 新定义字符串, 不改变原来的内容, 因strtok 会改变参数
    char *names_temp = (char *) malloc(strlen(elements_str) + 1);
    strcpy(names_temp, elements_str);

    int i = 0;
    char *token;
    char *sep = "|";
    token = strtok(names_temp, sep);
    while (token != NULL) {
        // 根据获取的长度重新分配内存大小
        elements[i] = (char *) realloc(elements[i], strlen(token) + 1);
        strcpy(elements[i], token);
        token = strtok(NULL, sep);
        i++;
    }
    free(names_temp);
}

/**
 * 根据obj类型判断是否继续执行geojson解析
 * @param obj obj
 * @param layer_count 图层个数
 * @param layer_names 外部定义的、指定的、需要处理的图层名数组, 可以为0
 * @param layer_name  当前对象所在的图层名, 长度限制300, 可在函数内部获取后在外部使用
 * @return
 */
bool ts_flag_geojson(Dwg_Object *obj, int layer_count, char *layer_names[], char *layer_name) {
    Dwg_Object_Entity *entity = obj->tio.entity;

    // 不处理1, 非实体对象
    if (obj->supertype != DWG_SUPERTYPE_ENTITY) {
        return false;
    }

    // 不处理2, 无图层属性
    Dwg_Object *layer = entity->layer ? entity->layer->obj : NULL;
    if (!layer || layer->type != DWG_TYPE_LAYER) {
        return false;
    }

    // 继续处理, 图层个数为0(即不指定图层), 继续执行解析geojson
    if (layer_count == 0) {
        return true;
    }

    // 判断是否在图层列表里
    bool flag_json = false;
    int layer_error;
    int name_length = 300;
    char *layer_temp = dwg_obj_table_get_name(layer, &layer_error);
//    strcpy(layer_name, layer_temp);
    strncpy(layer_name, layer_temp, name_length);
    for (int i = 0; i < layer_count; i++) {
        if (strcmp(layer_temp, layer_names[i]) == 0) {
            flag_json = true;
            break;
        }
    }
    // 2017版本需释放
    if (IS_FROM_TU_DWG (obj->parent)) {
        free(layer_temp);
    }                                           \
    return flag_json;
}

/**
 * 导出dwg文件为 geojson字符串
 * @param filename dwg文件名
 * @param layer_names 图层名,多个的话以竖线(|)分隔
 * @param block_names 块的名称,多个的话以竖线(|)分隔
 * @return geojson字符串
 */
char *ts_dwg_to_geojson(const char *filename, const char *layer_names_str, const char *block_names_str) {
    // 图层名拆分
    int layer_count = ts_element_count(layer_names_str);
    char *layer_names[layer_count];
    for (int i = 0; i < layer_count; i++) {
        // 在此处分配内存并初始化0, 在分割时重新分配大的内存.
        // 或固定长度, 后面有段代码就设置了固定长度300
        layer_names[i] = (char *) malloc(1);
        layer_names[i][0] = 0;
    }
    ts_element_sep(layer_names_str, layer_names);

    // 块名拆分
    int block_count = ts_element_count(block_names_str);
    char *block_names[block_count];
    for (int i = 0; i < block_count; i++) {
        // 在此处分配内存并初始化0, 在分割时重新分配大的内存.
        // 或固定长度, 后面有段代码就设置了固定长度300
        block_names[i] = (char *) malloc(1);
        block_names[i][0] = 0;
    }
    ts_element_sep(block_names_str, block_names);

    // geojson字符串, 默认10000, 后续动态分配
    size_t size_json = 10000;
    char *geojson = (char *) malloc(size_json);
    memset(geojson, 0, size_json);
    strcat(geojson, "{\"type\": \"FeatureCollection\",\"features\": [");

    // 读取文件、遍历对象
    Dwg_Data dwg;
    unsigned int opts = 0;
    dwg.opts = opts;
    dwg_read_file(filename, &dwg);
    for (unsigned i = 0; i < dwg.num_objects; i++) {
        Dwg_Object obj = dwg.object[i];

        // 判断是否继续执行，图层名限定300个字符
        char layer_name[300] = {0};
        if (!ts_flag_geojson(&obj, layer_count, layer_names, layer_name)) {
            continue;
        }

        // 判断块列表
        if (obj.type == DWG_TYPE_INSERT) {
            if (!ts_check_block(&obj, block_count, block_names)) {
                continue;
            }
        }

        // 读取1, 几何图形数据
        char *json_geometry = ts_object_json_geometry(&obj);
//        printf("%s \n", json_geometry);
        // 非单点(单块)、单线数据时，不处理了
        if (strlen(json_geometry) == 0) {
            free(json_geometry);
            continue;
        }

        // 读取2, 扩展属性数据
        char *json_eed = ts_object_json_eed(&obj, layer_name);
//        printf("%s \n", json_eed);

        // 是否动态增加空间, 额外增加100的空间存储geojson关键字
        size_t len_temp = strlen(geojson) + strlen(json_eed) + strlen(json_geometry) + 100;
        if (size_json < len_temp) {
            size_json += len_temp;
            geojson = (char *) realloc(geojson, size_json);
        }

        // 追加字符串
        strcat(geojson, "{\"type\": \"Feature\",");
        strcat(geojson, json_eed);
        strcat(geojson, ",");
        strcat(geojson, json_geometry);
        strcat(geojson, "},");

        // 释放内存
        free(json_eed);
        free(json_geometry);
    }

    // 删除尾部逗号(,)并追加闭合括号(]})
    geojson[strlen(geojson) - 1] = 0;
    strcat(geojson, "]}");
    // 释放内存
    for (int i = 0; i < layer_count; i++) {
        free(layer_names[i]);
    }
    dwg_free(&dwg);
    return geojson;
}

int main(int argc, char *argv[]) {
    const char *filename = "../examples-tssu/test-data/shitong.dwg";
//    const char *filename = "../examples-tssu/test-data昆山大地机械有限公司.dwg";
//    const char *filename = "../examples-tssu/test-data/ks-shi-simple2010.dwg";
//    const char *filename = "../examples-tssu/test-data/ks-shi.dwg";
//    const char *filename = "../examples-tssu/test-data/ks-kun.dwg";

    // 图层名-昆山市政、琨澄排水
    const char *layer_names = "GX_YSD|GX_YSL|GX_WSL|GX_WSD|YS|WS";

    // 块的名称
    const char *block_names = "";
//    const char *block_names = "排水井|化粪池|探测点|雨水井|WS-JCJFH|YS-JCJFH";

    char *geojson = ts_dwg_to_geojson(filename, layer_names, block_names);
    printf("%lu \n", strlen(geojson));
    printf("%s\n", geojson);
    free(geojson);
    return 0;
}
