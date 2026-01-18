#ifndef TINY_OBJ_LOADER_H_
#define TINY_OBJ_LOADER_H_
#include <map>
#include <string>
#include <vector>
namespace tinyobj {
    typedef struct {
        std::string name;
        float ambient[3];
        float diffuse[3];
        float specular[3];
        float transmittance[3];
        float emission[3];
        float shininess;
        float ior;       
        float dissolve;  
        int illum;
        int dummy;  
        std::string ambient_texname;             
        std::string diffuse_texname;             
        std::string specular_texname;            
        std::string specular_highlight_texname;  
        std::string bump_texname;                
        std::string displacement_texname;        
        std::string alpha_texname;               
        float roughness;                
        float metallic;                 
        float sheen;                    
        float clearcoat_thickness;      
        float clearcoat_roughness;      
        float anisotropy;               
        float anisotropy_rotation;      
        float pad0;
        std::string roughness_texname;  
        std::string metallic_texname;   
        std::string sheen_texname;      
        std::string emissive_texname;   
        std::string normal_texname;     
        std::map<std::string, std::string> unknown_parameter;
    } material_t;
    typedef struct {
        std::string name;
        std::vector<int> intValues;
        std::vector<float> floatValues;
        std::vector<std::string> stringValues;
    } tag_t;
    typedef struct {
        int vertex_index;
        int normal_index;
        int texcoord_index;
    } index_t;
    typedef struct {
        std::vector<index_t> indices;
        std::vector<unsigned char> num_face_vertices;  
        std::vector<int> material_ids;                 
        std::vector<tag_t> tags;                       
    } mesh_t;
    typedef struct {
        std::string name;
        mesh_t mesh;
    } shape_t;
    typedef struct {
        std::vector<float> vertices;   
        std::vector<float> normals;    
        std::vector<float> texcoords;  
    } attrib_t;
    typedef struct callback_t_ {
        void (*vertex_cb)(void *user_data, float x, float y, float z, float w);
        void (*normal_cb)(void *user_data, float x, float y, float z);
        void (*texcoord_cb)(void *user_data, float x, float y, float z);
        void (*index_cb)(void *user_data, index_t *indices, int num_indices);
        void (*usemtl_cb)(void *user_data, const char *name, int material_id);
        void (*mtllib_cb)(void *user_data, const material_t *materials,
                          int num_materials);
        void (*group_cb)(void *user_data, const char **names, int num_names);
        void (*object_cb)(void *user_data, const char *name);
        callback_t_()
        : vertex_cb(NULL),
        normal_cb(NULL),
        texcoord_cb(NULL),
        index_cb(NULL),
        usemtl_cb(NULL),
        mtllib_cb(NULL),
        group_cb(NULL),
        object_cb(NULL) {}
    } callback_t;
    class MaterialReader {
    public:
        MaterialReader() {}
        virtual ~MaterialReader();
        virtual bool operator()(const std::string &matId,
                                std::vector<material_t> *materials,
                                std::map<std::string, int> *matMap,
                                std::string *err) = 0;
    };
    class MaterialFileReader : public MaterialReader {
    public:
        explicit MaterialFileReader(const std::string &mtl_basepath)
        : m_mtlBasePath(mtl_basepath) {}
        virtual ~MaterialFileReader() {}
        virtual bool operator()(const std::string &matId,
                                std::vector<material_t> *materials,
                                std::map<std::string, int> *matMap, std::string *err);
    private:
        std::string m_mtlBasePath;
    };
    class MaterialStreamReader : public MaterialReader {
    public:
        explicit MaterialStreamReader(std::istream &inStream)
        : m_inStream(inStream) {}
        virtual ~MaterialStreamReader() {}
        virtual bool operator()(const std::string &matId,
                                std::vector<material_t> *materials,
                                std::map<std::string, int> *matMap, std::string *err);
    private:
        std::istream &m_inStream;
    };
    bool LoadObj(attrib_t *attrib, std::vector<shape_t> *shapes,
                 std::vector<material_t> *materials, std::string *err,
                 const char *filename, const char *mtl_basepath = NULL,
                 bool triangulate = true);
    bool LoadObjWithCallback(std::istream &inStream, const callback_t &callback,
                             void *user_data = NULL,
                             MaterialReader *readMatFn = NULL,
                             std::string *err = NULL);
    bool LoadObj(attrib_t *attrib, std::vector<shape_t> *shapes,
                 std::vector<material_t> *materials, std::string *err,
                 std::istream *inStream, MaterialReader *readMatFn = NULL,
                 bool triangulate = true);
    void LoadMtl(std::map<std::string, int> *material_map,
                 std::vector<material_t> *materials, std::istream *inStream);
}  
#ifdef TINYOBJLOADER_IMPLEMENTATION
#include <cassert>
#include <cctype>
#include <cmath>
#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <utility>
#include <fstream>
#include <sstream>
namespace tinyobj {
    MaterialReader::~MaterialReader() {}
#define TINYOBJ_SSCANF_BUFFER_SIZE (4096)
    struct vertex_index {
        int v_idx, vt_idx, vn_idx;
        vertex_index() : v_idx(-1), vt_idx(-1), vn_idx(-1) {}
        explicit vertex_index(int idx) : v_idx(idx), vt_idx(idx), vn_idx(idx) {}
        vertex_index(int vidx, int vtidx, int vnidx)
        : v_idx(vidx), vt_idx(vtidx), vn_idx(vnidx) {}
    };
    struct tag_sizes {
        tag_sizes() : num_ints(0), num_floats(0), num_strings(0) {}
        int num_ints;
        int num_floats;
        int num_strings;
    };
    struct obj_shape {
        std::vector<float> v;
        std::vector<float> vn;
        std::vector<float> vt;
    };
    static std::istream &safeGetline(std::istream &is, std::string &t) {
        t.clear();
        std::istream::sentry se(is, true);
        std::streambuf *sb = is.rdbuf();
        for (;;) {
            int c = sb->sbumpc();
            switch (c) {
                case '\n':
                    return is;
                case '\r':
                    if (sb->sgetc() == '\n') sb->sbumpc();
                    return is;
                case EOF:
                    if (t.empty()) is.setstate(std::ios::eofbit);
                    return is;
                default:
                    t += static_cast<char>(c);
            }
        }
    }
#define IS_SPACE(x) (((x) == ' ') || ((x) == '\t'))
#define IS_DIGIT(x) \
(static_cast<unsigned int>((x) - '0') < static_cast<unsigned int>(10))
#define IS_NEW_LINE(x) (((x) == '\r') || ((x) == '\n') || ((x) == '\0'))
    static inline int fixIndex(int idx, int n) {
        if (idx > 0) return idx - 1;
        if (idx == 0) return 0;
        return n + idx;  
    }
    static inline std::string parseString(const char **token) {
        std::string s;
        (*token) += strspn((*token), " \t");
        size_t e = strcspn((*token), " \t\r");
        s = std::string((*token), &(*token)[e]);
        (*token) += e;
        return s;
    }
    static inline int parseInt(const char **token) {
        (*token) += strspn((*token), " \t");
        int i = atoi((*token));
        (*token) += strcspn((*token), " \t\r");
        return i;
    }
    static bool tryParseDouble(const char *s, const char *s_end, double *result) {
        if (s >= s_end) {
            return false;
        }
        double mantissa = 0.0;
        int exponent = 0;
        char sign = '+';
        char exp_sign = '+';
        char const *curr = s;
        int read = 0;
        bool end_not_reached = false;
        if (*curr == '+' || *curr == '-') {
            sign = *curr;
            curr++;
        } else if (IS_DIGIT(*curr)) {  
        } else {
            goto fail;
        }
        end_not_reached = (curr != s_end);
        while (end_not_reached && IS_DIGIT(*curr)) {
            mantissa *= 10;
            mantissa += static_cast<int>(*curr - 0x30);
            curr++;
            read++;
            end_not_reached = (curr != s_end);
        }
        if (read == 0) goto fail;
        if (!end_not_reached) goto assemble;
        if (*curr == '.') {
            curr++;
            read = 1;
            end_not_reached = (curr != s_end);
            while (end_not_reached && IS_DIGIT(*curr)) {
                static const double pow_lut[] = {
                    1.0,
                    0.1,
                    0.01,
                    0.001,
                    0.0001,
                    0.00001,
                    0.000001,
                    0.0000001,
                };
                const int lut_entries = sizeof pow_lut / sizeof pow_lut[0];
                mantissa += static_cast<int>(*curr - 0x30) *
                (read < lut_entries ? pow_lut[read] : pow(10.0, -read));
                read++;
                curr++;
                end_not_reached = (curr != s_end);
            }
        } else if (*curr == 'e' || *curr == 'E') {
        } else {
            goto assemble;
        }
        if (!end_not_reached) goto assemble;
        if (*curr == 'e' || *curr == 'E') {
            curr++;
            end_not_reached = (curr != s_end);
            if (end_not_reached && (*curr == '+' || *curr == '-')) {
                exp_sign = *curr;
                curr++;
            } else if (IS_DIGIT(*curr)) {  
            } else {
                goto fail;
            }
            read = 0;
            end_not_reached = (curr != s_end);
            while (end_not_reached && IS_DIGIT(*curr)) {
                exponent *= 10;
                exponent += static_cast<int>(*curr - 0x30);
                curr++;
                read++;
                end_not_reached = (curr != s_end);
            }
            exponent *= (exp_sign == '+' ? 1 : -1);
            if (read == 0) goto fail;
        }
    assemble:
        *result = (sign == '+' ? 1 : -1) *
        (exponent ? ldexp(mantissa * pow(5.0, exponent), exponent) : mantissa);
        return true;
    fail:
        return false;
    }
    static inline float parseFloat(const char **token, double default_value = 0.0) {
        (*token) += strspn((*token), " \t");
        const char *end = (*token) + strcspn((*token), " \t\r");
        double val = default_value;
        tryParseDouble((*token), end, &val);
        float f = static_cast<float>(val);
        (*token) = end;
        return f;
    }
    static inline void parseFloat2(float *x, float *y, const char **token) {
        (*x) = parseFloat(token);
        (*y) = parseFloat(token);
    }
    static inline void parseFloat3(float *x, float *y, float *z,
                                   const char **token) {
        (*x) = parseFloat(token);
        (*y) = parseFloat(token);
        (*z) = parseFloat(token);
    }
    static inline void parseV(float *x, float *y, float *z, float *w,
                              const char **token) {
        (*x) = parseFloat(token);
        (*y) = parseFloat(token);
        (*z) = parseFloat(token);
        (*w) = parseFloat(token, 1.0);
    }
    static tag_sizes parseTagTriple(const char **token) {
        tag_sizes ts;
        ts.num_ints = atoi((*token));
        (*token) += strcspn((*token), "/ \t\r");
        if ((*token)[0] != '/') {
            return ts;
        }
        (*token)++;
        ts.num_floats = atoi((*token));
        (*token) += strcspn((*token), "/ \t\r");
        if ((*token)[0] != '/') {
            return ts;
        }
        (*token)++;
        ts.num_strings = atoi((*token));
        (*token) += strcspn((*token), "/ \t\r") + 1;
        return ts;
    }
    static vertex_index parseTriple(const char **token, int vsize, int vnsize,
                                    int vtsize) {
        vertex_index vi(-1);
        vi.v_idx = fixIndex(atoi((*token)), vsize);
        (*token) += strcspn((*token), "/ \t\r");
        if ((*token)[0] != '/') {
            return vi;
        }
        (*token)++;
        if ((*token)[0] == '/') {
            (*token)++;
            vi.vn_idx = fixIndex(atoi((*token)), vnsize);
            (*token) += strcspn((*token), "/ \t\r");
            return vi;
        }
        vi.vt_idx = fixIndex(atoi((*token)), vtsize);
        (*token) += strcspn((*token), "/ \t\r");
        if ((*token)[0] != '/') {
            return vi;
        }
        (*token)++;  
        vi.vn_idx = fixIndex(atoi((*token)), vnsize);
        (*token) += strcspn((*token), "/ \t\r");
        return vi;
    }
    static vertex_index parseRawTriple(const char **token) {
        vertex_index vi(static_cast<int>(0));  
        vi.v_idx = atoi((*token));
        (*token) += strcspn((*token), "/ \t\r");
        if ((*token)[0] != '/') {
            return vi;
        }
        (*token)++;
        if ((*token)[0] == '/') {
            (*token)++;
            vi.vn_idx = atoi((*token));
            (*token) += strcspn((*token), "/ \t\r");
            return vi;
        }
        vi.vt_idx = atoi((*token));
        (*token) += strcspn((*token), "/ \t\r");
        if ((*token)[0] != '/') {
            return vi;
        }
        (*token)++;  
        vi.vn_idx = atoi((*token));
        (*token) += strcspn((*token), "/ \t\r");
        return vi;
    }
    static void InitMaterial(material_t *material) {
        material->name = "";
        material->ambient_texname = "";
        material->diffuse_texname = "";
        material->specular_texname = "";
        material->specular_highlight_texname = "";
        material->bump_texname = "";
        material->displacement_texname = "";
        material->alpha_texname = "";
        for (int i = 0; i < 3; i++) {
            material->ambient[i] = 0.f;
            material->diffuse[i] = 0.f;
            material->specular[i] = 0.f;
            material->transmittance[i] = 0.f;
            material->emission[i] = 0.f;
        }
        material->illum = 0;
        material->dissolve = 1.f;
        material->shininess = 1.f;
        material->ior = 1.f;
        material->roughness = 0.f;
        material->metallic = 0.f;
        material->sheen = 0.f;
        material->clearcoat_thickness = 0.f;
        material->clearcoat_roughness = 0.f;
        material->anisotropy_rotation = 0.f;
        material->anisotropy = 0.f;
        material->roughness_texname = "";
        material->metallic_texname = "";
        material->sheen_texname = "";
        material->emissive_texname = "";
        material->normal_texname = "";
        material->unknown_parameter.clear();
    }
    static bool exportFaceGroupToShape(
                                       shape_t *shape, const std::vector<std::vector<vertex_index> > &faceGroup,
                                       const std::vector<tag_t> &tags, const int material_id,
                                       const std::string &name, bool triangulate) {
        if (faceGroup.empty()) {
            return false;
        }
        for (size_t i = 0; i < faceGroup.size(); i++) {
            const std::vector<vertex_index> &face = faceGroup[i];
            vertex_index i0 = face[0];
            vertex_index i1(-1);
            vertex_index i2 = face[1];
            size_t npolys = face.size();
            if (triangulate) {
                for (size_t k = 2; k < npolys; k++) {
                    i1 = i2;
                    i2 = face[k];
                    index_t idx0, idx1, idx2;
                    idx0.vertex_index = i0.v_idx;
                    idx0.normal_index = i0.vn_idx;
                    idx0.texcoord_index = i0.vt_idx;
                    idx1.vertex_index = i1.v_idx;
                    idx1.normal_index = i1.vn_idx;
                    idx1.texcoord_index = i1.vt_idx;
                    idx2.vertex_index = i2.v_idx;
                    idx2.normal_index = i2.vn_idx;
                    idx2.texcoord_index = i2.vt_idx;
                    shape->mesh.indices.push_back(idx0);
                    shape->mesh.indices.push_back(idx1);
                    shape->mesh.indices.push_back(idx2);
                    shape->mesh.num_face_vertices.push_back(3);
                    shape->mesh.material_ids.push_back(material_id);
                }
            } else {
                for (size_t k = 0; k < npolys; k++) {
                    index_t idx;
                    idx.vertex_index = face[k].v_idx;
                    idx.normal_index = face[k].vn_idx;
                    idx.texcoord_index = face[k].vt_idx;
                    shape->mesh.indices.push_back(idx);
                }
                shape->mesh.num_face_vertices.push_back(
                                                        static_cast<unsigned char>(npolys));
                shape->mesh.material_ids.push_back(material_id);  
            }
        }
        shape->name = name;
        shape->mesh.tags = tags;
        return true;
    }
    void LoadMtl(std::map<std::string, int> *material_map,
                 std::vector<material_t> *materials, std::istream *inStream) {
        material_t material;
        InitMaterial(&material);
        std::string linebuf;
        while (inStream->peek() != -1) {
            safeGetline(*inStream, linebuf);
            if (linebuf.size() > 0) {
                linebuf = linebuf.substr(0, linebuf.find_last_not_of(" \t") + 1);
            }
            if (linebuf.size() > 0) {
                if (linebuf[linebuf.size() - 1] == '\n')
                    linebuf.erase(linebuf.size() - 1);
            }
            if (linebuf.size() > 0) {
                if (linebuf[linebuf.size() - 1] == '\r')
                    linebuf.erase(linebuf.size() - 1);
            }
            if (linebuf.empty()) {
                continue;
            }
            const char *token = linebuf.c_str();
            token += strspn(token, " \t");
            assert(token);
            if (token[0] == '\0') continue;  
            if (token[0] == '#') continue;  
            if ((0 == strncmp(token, "newmtl", 6)) && IS_SPACE((token[6]))) {
                if (!material.name.empty()) {
                    material_map->insert(std::pair<std::string, int>(
                                                                     material.name, static_cast<int>(materials->size())));
                    materials->push_back(material);
                }
                InitMaterial(&material);
                char namebuf[TINYOBJ_SSCANF_BUFFER_SIZE];
                token += 7;
#ifdef _MSC_VER
                sscanf_s(token, "%s", namebuf, (unsigned)_countof(namebuf));
#else
                sscanf(token, "%s", namebuf);
#endif
                material.name = namebuf;
                continue;
            }
            if (token[0] == 'K' && token[1] == 'a' && IS_SPACE((token[2]))) {
                token += 2;
                float r, g, b;
                parseFloat3(&r, &g, &b, &token);
                material.ambient[0] = r;
                material.ambient[1] = g;
                material.ambient[2] = b;
                continue;
            }
            if (token[0] == 'K' && token[1] == 'd' && IS_SPACE((token[2]))) {
                token += 2;
                float r, g, b;
                parseFloat3(&r, &g, &b, &token);
                material.diffuse[0] = r;
                material.diffuse[1] = g;
                material.diffuse[2] = b;
                continue;
            }
            if (token[0] == 'K' && token[1] == 's' && IS_SPACE((token[2]))) {
                token += 2;
                float r, g, b;
                parseFloat3(&r, &g, &b, &token);
                material.specular[0] = r;
                material.specular[1] = g;
                material.specular[2] = b;
                continue;
            }
            if ((token[0] == 'K' && token[1] == 't' && IS_SPACE((token[2]))) ||
                (token[0] == 'T' && token[1] == 'f' && IS_SPACE((token[2])))) {
                token += 2;
                float r, g, b;
                parseFloat3(&r, &g, &b, &token);
                material.transmittance[0] = r;
                material.transmittance[1] = g;
                material.transmittance[2] = b;
                continue;
            }
            if (token[0] == 'N' && token[1] == 'i' && IS_SPACE((token[2]))) {
                token += 2;
                material.ior = parseFloat(&token);
                continue;
            }
            if (token[0] == 'K' && token[1] == 'e' && IS_SPACE(token[2])) {
                token += 2;
                float r, g, b;
                parseFloat3(&r, &g, &b, &token);
                material.emission[0] = r;
                material.emission[1] = g;
                material.emission[2] = b;
                continue;
            }
            if (token[0] == 'N' && token[1] == 's' && IS_SPACE(token[2])) {
                token += 2;
                material.shininess = parseFloat(&token);
                continue;
            }
            if (0 == strncmp(token, "illum", 5) && IS_SPACE(token[5])) {
                token += 6;
                material.illum = parseInt(&token);
                continue;
            }
            if ((token[0] == 'd' && IS_SPACE(token[1]))) {
                token += 1;
                material.dissolve = parseFloat(&token);
                continue;
            }
            if (token[0] == 'T' && token[1] == 'r' && IS_SPACE(token[2])) {
                token += 2;
                material.dissolve = 1.0f - parseFloat(&token);
                continue;
            }
            if (token[0] == 'P' && token[1] == 'r' && IS_SPACE(token[2])) {
                token += 2;
                material.roughness = parseFloat(&token);
                continue;
            }
            if (token[0] == 'P' && token[1] == 'm' && IS_SPACE(token[2])) {
                token += 2;
                material.metallic = parseFloat(&token);
                continue;
            }
            if (token[0] == 'P' && token[1] == 's' && IS_SPACE(token[2])) {
                token += 2;
                material.sheen = parseFloat(&token);
                continue;
            }
            if (token[0] == 'P' && token[1] == 'c' && IS_SPACE(token[2])) {
                token += 2;
                material.clearcoat_thickness = parseFloat(&token);
                continue;
            }
            if ((0 == strncmp(token, "Pcr", 3)) && IS_SPACE(token[3])) {
                token += 4;
                material.clearcoat_roughness = parseFloat(&token);
                continue;
            }
            if ((0 == strncmp(token, "aniso", 5)) && IS_SPACE(token[5])) {
                token += 6;
                material.anisotropy = parseFloat(&token);
                continue;
            }
            if ((0 == strncmp(token, "anisor", 6)) && IS_SPACE(token[6])) {
                token += 7;
                material.anisotropy_rotation = parseFloat(&token);
                continue;
            }
            if ((0 == strncmp(token, "map_Ka", 6)) && IS_SPACE(token[6])) {
                token += 7;
                material.ambient_texname = token;
                continue;
            }
            if ((0 == strncmp(token, "map_Kd", 6)) && IS_SPACE(token[6])) {
                token += 7;
                material.diffuse_texname = token;
                continue;
            }
            if ((0 == strncmp(token, "map_Ks", 6)) && IS_SPACE(token[6])) {
                token += 7;
                material.specular_texname = token;
                continue;
            }
            if ((0 == strncmp(token, "map_Ns", 6)) && IS_SPACE(token[6])) {
                token += 7;
                material.specular_highlight_texname = token;
                continue;
            }
            if ((0 == strncmp(token, "map_bump", 8)) && IS_SPACE(token[8])) {
                token += 9;
                material.bump_texname = token;
                continue;
            }
            if ((0 == strncmp(token, "map_d", 5)) && IS_SPACE(token[5])) {
                token += 6;
                material.alpha_texname = token;
                continue;
            }
            if ((0 == strncmp(token, "bump", 4)) && IS_SPACE(token[4])) {
                token += 5;
                material.bump_texname = token;
                continue;
            }
            if ((0 == strncmp(token, "disp", 4)) && IS_SPACE(token[4])) {
                token += 5;
                material.displacement_texname = token;
                continue;
            }
            if ((0 == strncmp(token, "map_Pr", 6)) && IS_SPACE(token[6])) {
                token += 7;
                material.roughness_texname = token;
                continue;
            }
            if ((0 == strncmp(token, "map_Pm", 6)) && IS_SPACE(token[6])) {
                token += 7;
                material.metallic_texname = token;
                continue;
            }
            if ((0 == strncmp(token, "map_Ps", 6)) && IS_SPACE(token[6])) {
                token += 7;
                material.sheen_texname = token;
                continue;
            }
            if ((0 == strncmp(token, "map_Ke", 6)) && IS_SPACE(token[6])) {
                token += 7;
                material.emissive_texname = token;
                continue;
            }
            if ((0 == strncmp(token, "norm", 4)) && IS_SPACE(token[4])) {
                token += 5;
                material.normal_texname = token;
                continue;
            }
            const char *_space = strchr(token, ' ');
            if (!_space) {
                _space = strchr(token, '\t');
            }
            if (_space) {
                std::ptrdiff_t len = _space - token;
                std::string key(token, static_cast<size_t>(len));
                std::string value = _space + 1;
                material.unknown_parameter.insert(
                                                  std::pair<std::string, std::string>(key, value));
            }
        }
        material_map->insert(std::pair<std::string, int>(
                                                         material.name, static_cast<int>(materials->size())));
        materials->push_back(material);
    }
    bool MaterialFileReader::operator()(const std::string &matId,
                                        std::vector<material_t> *materials,
                                        std::map<std::string, int> *matMap,
                                        std::string *err) {
        std::string filepath;
        if (!m_mtlBasePath.empty()) {
            filepath = std::string(m_mtlBasePath) + matId;
        } else {
            filepath = matId;
        }
        std::ifstream matIStream(filepath.c_str());
        LoadMtl(matMap, materials, &matIStream);
        if (!matIStream) {
            std::stringstream ss;
            ss << "WARN: Material file [ " << filepath
            << " ] not found. Created a default material.";
            if (err) {
                (*err) += ss.str();
            }
        }
        return true;
    }
    bool MaterialStreamReader::operator()(const std::string &matId,
                                          std::vector<material_t> *materials,
                                          std::map<std::string, int> *matMap,
                                          std::string *err) {
        (void)matId;
        LoadMtl(matMap, materials, &m_inStream);
        if (!m_inStream) {
            std::stringstream ss;
            ss << "WARN: Material stream in error state."
            << " Created a default material.";
            if (err) {
                (*err) += ss.str();
            }
        }
        return true;
    }
    bool LoadObj(attrib_t *attrib, std::vector<shape_t> *shapes,
                 std::vector<material_t> *materials, std::string *err,
                 const char *filename, const char *mtl_basepath,
                 bool trianglulate) {
        attrib->vertices.clear();
        attrib->normals.clear();
        attrib->texcoords.clear();
        shapes->clear();
        std::stringstream errss;
        std::ifstream ifs(filename);
        if (!ifs) {
            errss << "Cannot open file [" << filename << "]" << std::endl;
            if (err) {
                (*err) = errss.str();
            }
            return false;
        }
        std::string basePath;
        if (mtl_basepath) {
            basePath = mtl_basepath;
        }
        MaterialFileReader matFileReader(basePath);
        return LoadObj(attrib, shapes, materials, err, &ifs, &matFileReader,
                       trianglulate);
    }
    bool LoadObj(attrib_t *attrib, std::vector<shape_t> *shapes,
                 std::vector<material_t> *materials, std::string *err,
                 std::istream *inStream,
                 MaterialReader *readMatFn  ,
                 bool triangulate) {
        std::stringstream errss;
        std::vector<float> v;
        std::vector<float> vn;
        std::vector<float> vt;
        std::vector<tag_t> tags;
        std::vector<std::vector<vertex_index> > faceGroup;
        std::string name;
        std::map<std::string, int> material_map;
        int material = -1;
        shape_t shape;
        std::string linebuf;
        while (inStream->peek() != -1) {
            safeGetline(*inStream, linebuf);
            if (linebuf.size() > 0) {
                if (linebuf[linebuf.size() - 1] == '\n')
                    linebuf.erase(linebuf.size() - 1);
            }
            if (linebuf.size() > 0) {
                if (linebuf[linebuf.size() - 1] == '\r')
                    linebuf.erase(linebuf.size() - 1);
            }
            if (linebuf.empty()) {
                continue;
            }
            const char *token = linebuf.c_str();
            token += strspn(token, " \t");
            assert(token);
            if (token[0] == '\0') continue;  
            if (token[0] == '#') continue;  
            if (token[0] == 'v' && IS_SPACE((token[1]))) {
                token += 2;
                float x, y, z;
                parseFloat3(&x, &y, &z, &token);
                v.push_back(x);
                v.push_back(y);
                v.push_back(z);
                continue;
            }
            if (token[0] == 'v' && token[1] == 'n' && IS_SPACE((token[2]))) {
                token += 3;
                float x, y, z;
                parseFloat3(&x, &y, &z, &token);
                vn.push_back(x);
                vn.push_back(y);
                vn.push_back(z);
                continue;
            }
            if (token[0] == 'v' && token[1] == 't' && IS_SPACE((token[2]))) {
                token += 3;
                float x, y;
                parseFloat2(&x, &y, &token);
                vt.push_back(x);
                vt.push_back(y);
                continue;
            }
            if (token[0] == 'f' && IS_SPACE((token[1]))) {
                token += 2;
                token += strspn(token, " \t");
                std::vector<vertex_index> face;
                face.reserve(3);
                while (!IS_NEW_LINE(token[0])) {
                    vertex_index vi = parseTriple(&token, static_cast<int>(v.size() / 3),
                                                  static_cast<int>(vn.size() / 3),
                                                  static_cast<int>(vt.size() / 2));
                    face.push_back(vi);
                    size_t n = strspn(token, " \t\r");
                    token += n;
                }
                faceGroup.push_back(std::vector<vertex_index>());
                faceGroup[faceGroup.size() - 1].swap(face);
                continue;
            }
            if ((0 == strncmp(token, "usemtl", 6)) && IS_SPACE((token[6]))) {
                char namebuf[TINYOBJ_SSCANF_BUFFER_SIZE];
                token += 7;
#ifdef _MSC_VER
                sscanf_s(token, "%s", namebuf, (unsigned)_countof(namebuf));
#else
                sscanf(token, "%s", namebuf);
#endif
                int newMaterialId = -1;
                if (material_map.find(namebuf) != material_map.end()) {
                    newMaterialId = material_map[namebuf];
                } else {
                }
                if (newMaterialId != material) {
                    exportFaceGroupToShape(&shape, faceGroup, tags, material, name,
                                           triangulate);
                    faceGroup.clear();
                    material = newMaterialId;
                }
                continue;
            }
            if ((0 == strncmp(token, "mtllib", 6)) && IS_SPACE((token[6]))) {
                if (readMatFn) {
                    char namebuf[TINYOBJ_SSCANF_BUFFER_SIZE];
                    token += 7;
#ifdef _MSC_VER
                    sscanf_s(token, "%s", namebuf, (unsigned)_countof(namebuf));
#else
                    sscanf(token, "%s", namebuf);
#endif
                    std::string err_mtl;
                    bool ok = (*readMatFn)(namebuf, materials, &material_map, &err_mtl);
                    if (err) {
                        (*err) += err_mtl;
                    }
                    if (!ok) {
                        faceGroup.clear();  
                        return false;
                    }
                }
                continue;
            }
            if (token[0] == 'g' && IS_SPACE((token[1]))) {
                bool ret = exportFaceGroupToShape(&shape, faceGroup, tags, material, name,
                                                  triangulate);
                if (ret) {
                    shapes->push_back(shape);
                }
                shape = shape_t();
                faceGroup.clear();
                std::vector<std::string> names;
                names.reserve(2);
                while (!IS_NEW_LINE(token[0])) {
                    std::string str = parseString(&token);
                    names.push_back(str);
                    token += strspn(token, " \t\r");  
                }
                assert(names.size() > 0);
                if (names.size() > 1) {
                    name = names[1];
                } else {
                    name = "";
                }
                continue;
            }
            if (token[0] == 'o' && IS_SPACE((token[1]))) {
                bool ret = exportFaceGroupToShape(&shape, faceGroup, tags, material, name,
                                                  triangulate);
                if (ret) {
                    shapes->push_back(shape);
                }
                faceGroup.clear();
                shape = shape_t();
                char namebuf[TINYOBJ_SSCANF_BUFFER_SIZE];
                token += 2;
#ifdef _MSC_VER
                sscanf_s(token, "%s", namebuf, (unsigned)_countof(namebuf));
#else
                sscanf(token, "%s", namebuf);
#endif
                name = std::string(namebuf);
                continue;
            }
            if (token[0] == 't' && IS_SPACE(token[1])) {
                tag_t tag;
                char namebuf[4096];
                token += 2;
#ifdef _MSC_VER
                sscanf_s(token, "%s", namebuf, (unsigned)_countof(namebuf));
#else
                sscanf(token, "%s", namebuf);
#endif
                tag.name = std::string(namebuf);
                token += tag.name.size() + 1;
                tag_sizes ts = parseTagTriple(&token);
                tag.intValues.resize(static_cast<size_t>(ts.num_ints));
                for (size_t i = 0; i < static_cast<size_t>(ts.num_ints); ++i) {
                    tag.intValues[i] = atoi(token);
                    token += strcspn(token, "/ \t\r") + 1;
                }
                tag.floatValues.resize(static_cast<size_t>(ts.num_floats));
                for (size_t i = 0; i < static_cast<size_t>(ts.num_floats); ++i) {
                    tag.floatValues[i] = parseFloat(&token);
                    token += strcspn(token, "/ \t\r") + 1;
                }
                tag.stringValues.resize(static_cast<size_t>(ts.num_strings));
                for (size_t i = 0; i < static_cast<size_t>(ts.num_strings); ++i) {
                    char stringValueBuffer[4096];
#ifdef _MSC_VER
                    sscanf_s(token, "%s", stringValueBuffer,
                             (unsigned)_countof(stringValueBuffer));
#else
                    sscanf(token, "%s", stringValueBuffer);
#endif
                    tag.stringValues[i] = stringValueBuffer;
                    token += tag.stringValues[i].size() + 1;
                }
                tags.push_back(tag);
            }
        }
        bool ret = exportFaceGroupToShape(&shape, faceGroup, tags, material, name,
                                          triangulate);
        if (ret || shape.mesh.indices.size()) {
            shapes->push_back(shape);
        }
        faceGroup.clear();  
        if (err) {
            (*err) += errss.str();
        }
        attrib->vertices.swap(v);
        attrib->normals.swap(vn);
        attrib->texcoords.swap(vt);
        return true;
    }
    bool LoadObjWithCallback(std::istream &inStream, const callback_t &callback,
                             void *user_data  ,
                             MaterialReader *readMatFn  ,
                             std::string *err  ) {
        std::stringstream errss;
        std::map<std::string, int> material_map;
        int material_id = -1;  
        std::vector<index_t> indices;
        std::vector<material_t> materials;
        std::vector<std::string> names;
        names.reserve(2);
        std::string name;
        std::vector<const char *> names_out;
        std::string linebuf;
        while (inStream.peek() != -1) {
            safeGetline(inStream, linebuf);
            if (linebuf.size() > 0) {
                if (linebuf[linebuf.size() - 1] == '\n')
                    linebuf.erase(linebuf.size() - 1);
            }
            if (linebuf.size() > 0) {
                if (linebuf[linebuf.size() - 1] == '\r')
                    linebuf.erase(linebuf.size() - 1);
            }
            if (linebuf.empty()) {
                continue;
            }
            const char *token = linebuf.c_str();
            token += strspn(token, " \t");
            assert(token);
            if (token[0] == '\0') continue;  
            if (token[0] == '#') continue;  
            if (token[0] == 'v' && IS_SPACE((token[1]))) {
                token += 2;
                float x, y, z, w;  
                parseV(&x, &y, &z, &w, &token);
                if (callback.vertex_cb) {
                    callback.vertex_cb(user_data, x, y, z, w);
                }
                continue;
            }
            if (token[0] == 'v' && token[1] == 'n' && IS_SPACE((token[2]))) {
                token += 3;
                float x, y, z;
                parseFloat3(&x, &y, &z, &token);
                if (callback.normal_cb) {
                    callback.normal_cb(user_data, x, y, z);
                }
                continue;
            }
            if (token[0] == 'v' && token[1] == 't' && IS_SPACE((token[2]))) {
                token += 3;
                float x, y, z;  
                parseFloat3(&x, &y, &z, &token);
                if (callback.texcoord_cb) {
                    callback.texcoord_cb(user_data, x, y, z);
                }
                continue;
            }
            if (token[0] == 'f' && IS_SPACE((token[1]))) {
                token += 2;
                token += strspn(token, " \t");
                indices.clear();
                while (!IS_NEW_LINE(token[0])) {
                    vertex_index vi = parseRawTriple(&token);
                    index_t idx;
                    idx.vertex_index = vi.v_idx;
                    idx.normal_index = vi.vn_idx;
                    idx.texcoord_index = vi.vt_idx;
                    indices.push_back(idx);
                    size_t n = strspn(token, " \t\r");
                    token += n;
                }
                if (callback.index_cb && indices.size() > 0) {
                    callback.index_cb(user_data, &indices.at(0),
                                      static_cast<int>(indices.size()));
                }
                continue;
            }
            if ((0 == strncmp(token, "usemtl", 6)) && IS_SPACE((token[6]))) {
                char namebuf[TINYOBJ_SSCANF_BUFFER_SIZE];
                token += 7;
#ifdef _MSC_VER
                sscanf_s(token, "%s", namebuf,
                         static_cast<unsigned int>(_countof(namebuf)));
#else
                sscanf(token, "%s", namebuf);
#endif
                int newMaterialId = -1;
                if (material_map.find(namebuf) != material_map.end()) {
                    newMaterialId = material_map[namebuf];
                } else {
                }
                if (newMaterialId != material_id) {
                    material_id = newMaterialId;
                }
                if (callback.usemtl_cb) {
                    callback.usemtl_cb(user_data, namebuf, material_id);
                }
                continue;
            }
            if ((0 == strncmp(token, "mtllib", 6)) && IS_SPACE((token[6]))) {
                if (readMatFn) {
                    char namebuf[TINYOBJ_SSCANF_BUFFER_SIZE];
                    token += 7;
#ifdef _MSC_VER
                    sscanf_s(token, "%s", namebuf, (unsigned)_countof(namebuf));
#else
                    sscanf(token, "%s", namebuf);
#endif
                    std::string err_mtl;
                    materials.clear();
                    bool ok = (*readMatFn)(namebuf, &materials, &material_map, &err_mtl);
                    if (err) {
                        (*err) += err_mtl;
                    }
                    if (!ok) {
                        return false;
                    }
                    if (callback.mtllib_cb) {
                        callback.mtllib_cb(user_data, &materials.at(0),
                                           static_cast<int>(materials.size()));
                    }
                }
                continue;
            }
            if (token[0] == 'g' && IS_SPACE((token[1]))) {
                names.clear();
                while (!IS_NEW_LINE(token[0])) {
                    std::string str = parseString(&token);
                    names.push_back(str);
                    token += strspn(token, " \t\r");  
                }
                assert(names.size() > 0);
                if (names.size() > 1) {
                    name = names[1];
                } else {
                    name.clear();
                }
                if (callback.group_cb) {
                    if (names.size() > 1) {
                        names_out.resize(names.size() - 1);
                        for (size_t j = 0; j < names_out.size(); j++) {
                            names_out[j] = names[j + 1].c_str();
                        }
                        callback.group_cb(user_data, &names_out.at(0),
                                          static_cast<int>(names_out.size()));
                    } else {
                        callback.group_cb(user_data, NULL, 0);
                    }
                }
                continue;
            }
            if (token[0] == 'o' && IS_SPACE((token[1]))) {
                char namebuf[TINYOBJ_SSCANF_BUFFER_SIZE];
                token += 2;
#ifdef _MSC_VER
                sscanf_s(token, "%s", namebuf, (unsigned)_countof(namebuf));
#else
                sscanf(token, "%s", namebuf);
#endif
                std::string object_name = std::string(namebuf);
                if (callback.object_cb) {
                    callback.object_cb(user_data, object_name.c_str());
                }
                continue;
            }
#if 0  
            if (token[0] == 't' && IS_SPACE(token[1])) {
                tag_t tag;
                char namebuf[4096];
                token += 2;
#ifdef _MSC_VER
                sscanf_s(token, "%s", namebuf, (unsigned)_countof(namebuf));
#else
                sscanf(token, "%s", namebuf);
#endif
                tag.name = std::string(namebuf);
                token += tag.name.size() + 1;
                tag_sizes ts = parseTagTriple(&token);
                tag.intValues.resize(static_cast<size_t>(ts.num_ints));
                for (size_t i = 0; i < static_cast<size_t>(ts.num_ints); ++i) {
                    tag.intValues[i] = atoi(token);
                    token += strcspn(token, "/ \t\r") + 1;
                }
                tag.floatValues.resize(static_cast<size_t>(ts.num_floats));
                for (size_t i = 0; i < static_cast<size_t>(ts.num_floats); ++i) {
                    tag.floatValues[i] = parseFloat(&token);
                    token += strcspn(token, "/ \t\r") + 1;
                }
                tag.stringValues.resize(static_cast<size_t>(ts.num_strings));
                for (size_t i = 0; i < static_cast<size_t>(ts.num_strings); ++i) {
                    char stringValueBuffer[4096];
#ifdef _MSC_VER
                    sscanf_s(token, "%s", stringValueBuffer,
                             (unsigned)_countof(stringValueBuffer));
#else
                    sscanf(token, "%s", stringValueBuffer);
#endif
                    tag.stringValues[i] = stringValueBuffer;
                    token += tag.stringValues[i].size() + 1;
                }
                tags.push_back(tag);
            }
#endif
        }
        if (err) {
            (*err) += errss.str();
        }
        return true;
    }
}  
#endif
#endif  