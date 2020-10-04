#include <cvx/viz/scene/material.hpp>
#include <cvx/util/misc/format.hpp>
#include <Eigen/Core>
#include <Eigen/Geometry>

#include <GL/gl3w.h>

using namespace std ;
using namespace Eigen ;
using namespace cvx::util ;

string vertex_shader_code =
        R"(
        layout (location = 0) in vec3 vposition;
        out vec3 position;
#line 1

#ifdef HAS_NORMALS
        layout (location = 1) in vec3 vnormal;
        out vec3 normal;
#endif

#ifdef HAS_COLORS
        layout (location = 2) in vec3 vcolor;
        out vec3 color ;
#endif

#ifdef HAS_SHADOWS
        out vec4 lspos ;
#endif

#ifdef USE_SKINNING
        layout (location = 3) in ivec4 boneIDs;
        layout (location = 4) in vec4  boneWeights;

        const int MAX_BONES = 200;

        uniform mat4 g_bones[MAX_BONES];
#endif

#ifdef  HAS_UVs
        layout (location = 5) in vec2 vuv;
        out vec2 uv;
#endif
        uniform mat4 model ;
        uniform mat4 mvp;
        uniform mat4 mv;
        uniform mat3 mvn ;

#ifdef HAS_SHADOWS
        uniform mat4 mls ;
#endif

        void main()
        {
#ifdef USE_SKINNING
        mat4 BoneTransform = g_bones[boneIDs[0]] * boneWeights[0];
        BoneTransform     += g_bones[boneIDs[1]] * boneWeights[1];
        BoneTransform     += g_bones[boneIDs[2]] * boneWeights[2];
        BoneTransform     += g_bones[boneIDs[3]] * boneWeights[3];

        vec4 posl    = BoneTransform * vec4(vposition, 1.0);

#ifdef HAS_NORMALS
        vec3 normall = mat3(BoneTransform) * vnormal;
#endif

#else
        vec4 posl = vec4(vposition, 1.0);

#ifdef HAS_NORMALS
        vec3 normall = vnormal;
#endif

#endif // SKINING

        gl_Position  = mvp * posl;

#ifdef HAS_NORMALS
        normal = mvn * normall;
#endif

#ifdef HAS_SHADOWS
        vec3 fpos = vec3(model * posl);
        lspos = mls * vec4(fpos, 1);
#endif

#ifdef HAS_COLORS
       color = vcolor ;
#endif

#ifdef HAS_UVs
        uv = vuv ;
#endif
        position    = (mv * posl).xyz;
}
)";

static string pn_vertex_shader_code =
        R"(
        layout (location = 0) in vec3 vposition;
        layout (location = 1) in vec3 vnormal;

        out vec3 position;
        out vec3 normal;

        uniform mat4 mvp;
        uniform mat4 mv;
        uniform mat3 mvn ;

        void main()
        {
        vec4 posl    = vec4(vposition, 1.0);
        gl_Position  = mvp * posl;
        normal       = mvn * vnormal;
        position    = (mv * posl).xyz;
        }
        )";

static string pnt_vertex_shader_code =
        R"(

        layout (location = 0) in vec3 vposition;
        layout (location = 1) in vec3 vnormal;
        layout (location = 5) in vec2 vuv;

        out vec3 position;
        out vec3 normal;
        out vec2 uv;

        uniform mat4 mvp;
        uniform mat4 mv;
        uniform mat3 mvn ;

        void main()
        {
        vec4 posl    = vec4(vposition, 1.0);
        gl_Position  = mvp * posl;
        normal       = mvn * vnormal;
        position    = (mv * posl).xyz;
        uv = vuv ;
        }
        )";

static string p_vertex_shader_code =
        R"(
     layout (location = 0) in vec3 vposition;

        uniform mat4 mvp;
        uniform mat4 mv;
        uniform mat3 mvn ;

        void main()
        {
        vec4 posl    = vec4(vposition, 1.0);
        gl_Position  = mvp * posl;
        }
        )";

static string pc_vertex_shader_code =
        R"(
        layout (location = 0) in vec3 vposition;
        layout (location = 2) in vec3 vcolor;

        out vec3 color ;

        uniform mat4 mvp;
        uniform mat4 mv;
        uniform mat3 mvn ;

        void main()
        {
        vec4 posl    = vec4(vposition, 1.0);
        gl_Position  = mvp * posl;
        color = vcolor ;
        }
        )";

static string phong_fragment_shader_common =
        R"(
        precision mediump float;
        in vec3 normal;
        in vec3 position;
#ifdef HAS_SHADOWS
        in vec4 lspos ;
#endif

        const int MAX_LIGHTS = 10;
        const int AMBIENT_LIGHT = 0 ;
        const int DIRECTIONAL_LIGHT = 1 ;
        const int SPOT_LIGHT = 2 ;
        const int POINT_LIGHT = 3 ;

        struct LightSourceParameters
        {
        int light_type ;
        vec3 color;
        vec4 position;
        vec3 direction;
        float spot_exponent;
        float spot_cos_cutoff;
        float constant_attenuation;
        float linear_attenuation;
        float quadratic_attenuation;
        };

        uniform LightSourceParameters g_light_source[MAX_LIGHTS];

        struct MaterialParameters
        {
        vec4 ambient;     // Acm
        vec4 diffuse;     // Dcm
        vec4 specular;    // Scm
        float shininess;  // Srm
        };

        uniform MaterialParameters g_material;

#ifdef HAS_SHADOWS
        uniform sampler2D shadowMap ;
        uniform float shadowBias ;

        float ShadowCalculation(vec4 fragPosLightSpace)
        {
            // perform perspective divide
            vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;


            // transform to [0,1] range
            projCoords = projCoords * 0.5 + 0.5;

        if ( projCoords.z > 1.0 ) return 0.0 ;
        float shadow = 0.0;
        vec2 texelSize = 1.0 / textureSize(shadowMap, 0);

         float currentDepth = projCoords.z;
        for(int x = -1; x <= 1; ++x)
        {
            for(int y = -1; y <= 1; ++y)
            {
                float pcfDepth = texture(shadowMap, projCoords.xy + vec2(x, y) * texelSize).r;
                shadow += currentDepth - shadowBias > pcfDepth ? 1.0 : 0.0;
            }
        }
        shadow /= 9.0;

            return shadow;
        }

#endif

     out vec4 FragColor;

     vec4 phongIllumination(vec4 dc) {
        vec3 N = normalize(normal);
        vec4 finalColor = vec4(0, 0, 0, 1.0);

        float shadow  = 0.0 ;
#ifdef HAS_SHADOWS
        shadow = 0.5*ShadowCalculation(lspos);
#endif

        for (int i=0;i<MAX_LIGHTS;i++)
        {
        vec3 L ;
        float att = 1.0 ;

        if ( g_light_source[i].light_type == -1 ) continue ;
        else if ( g_light_source[i].light_type == AMBIENT_LIGHT ) {
        finalColor += vec4(g_light_source[i].color, 1.0) * g_material.ambient ;
        continue ;
        }
        else if ( g_light_source[i].light_type == DIRECTIONAL_LIGHT ) {
        L = normalize(g_light_source[i].direction) ;
        }
        else if ( g_light_source[i].light_type == SPOT_LIGHT ) {
        float dist = length(g_light_source[i].position.xyz - position) ;
        L = normalize(g_light_source[i].position.xyz - position);

        float spotEffect = dot(normalize(g_light_source[i].direction), normalize(-L));
        if (spotEffect > g_light_source[i].spot_cos_cutoff) {
        spotEffect = pow(spotEffect, g_light_source[i].spot_exponent);
        att = spotEffect / (g_light_source[i].constant_attenuation +
        g_light_source[i].linear_attenuation * dist +
        g_light_source[i].quadratic_attenuation * dist * dist);

        }
        else att = 0.0 ;
        }
        else if ( g_light_source[i].light_type == POINT_LIGHT ) {
        float dist = length(g_light_source[i].position.xyz - position);
        L = normalize(g_light_source[i].position.xyz - position);

        att = 1.0 / (g_light_source[i].constant_attenuation +
        g_light_source[i].linear_attenuation * dist +
        g_light_source[i].quadratic_attenuation * dist * dist);

        }

        vec3 E = normalize(-position); // we are in Eye Coordinates, so EyePos is (0,0,0)
        vec3 R = normalize(-reflect(L,N));

        //calculate Diffuse Term:

        vec4 Idiff = vec4(g_light_source[i].color, 1.0) * dc * max(dot(N,L), 0.0f);
        Idiff = clamp(Idiff, 0.0, 1.0);

        // calculate Specular Term:
        vec4 Ispec = vec4(g_light_source[i].color, 1.0) * g_material.specular
        * pow(max(dot(R,E),0.0f),g_material.shininess);
        Ispec = clamp(Ispec, 0.0, 1.0);

        finalColor +=  att*clamp((1 - shadow)*(Ispec + Idiff), 0.0, 1.0);

        }

        return  finalColor ;
        }
        )";

static string phong_fragment_shader_material = R"(
                                               void main (void)
                                               {
                                               FragColor = phongIllumination(g_material.diffuse);
                                               }
                                               )";

static string phong_fragment_shader_map = R"(
                                          in vec2 uv ;
                                          uniform sampler2D tex_unit;

                                          void main (void)
                                          {
                                          FragColor = phongIllumination(texture(tex_unit, uv));
                                          }
                                          )";

static string constant_fragment_shader = R"(

                                         uniform vec4 g_material_clr ;
                                         out vec4 FragColor;

                                         void main (void)
                                         {
                                         FragColor = g_material_clr ;
                                         }
                                         )";

static string pervertex_fragment_shader = R"(


                                          in vec3 color ;
                                          out vec4 FragColor;
                                          uniform float opacity ;

                                          void main (void)
                                          {
                                          FragColor = vec4(color, opacity) ;
                                          }
                                          )";

namespace cvx { namespace viz {

using MaterialPtr = std::shared_ptr<Material> ;


class PhongMaterial: public Material {
public:

    PhongMaterial(int flags): Material(flags) {}

    gl::ShaderProgram::Ptr prog() override ;

    static MaterialPtr instance(int flags) {
        static std::map<int, MaterialPtr> s_materials ;
        return materialSingleton<PhongMaterial>(s_materials, flags) ;
    }

    gl::ShaderProgram::Ptr prog_ ;
};

static const char *version_header = "#version 330\n" ;

gl::ShaderProgram::Ptr PhongMaterial::prog()
{
    if ( prog_ ) return prog_ ;

    gl::Shader::Ptr vs(new gl::Shader(gl::Shader::Vertex)) ;

    std::string preproc ;
    preproc.append("#define HAS_NORMALS\n") ;
    if ( flags_ & USE_SKINNING )  preproc.append("#define USE_SKINNING\n") ;
    if ( flags_ & HAS_SHADOWS )  preproc.append("#define HAS_SHADOWS\n") ;

    vs->addSourceString(version_header) ;
    vs->addSourceString(preproc) ;
    vs->addSourceString(vertex_shader_code, "vertex_shader_code") ;

    gl::Shader::Ptr fs(new gl::Shader(gl::Shader::Fragment)) ;

    fs->addSourceString(version_header) ;
    fs->addSourceString(preproc) ;
    fs->addSourceString(phong_fragment_shader_common) ;
    fs->addSourceString(phong_fragment_shader_material, "phong_fragment_shader_material")  ;

    prog_.reset(new gl::ShaderProgram) ;
    prog_->addShader(vs) ;
    prog_->addShader(fs) ;

    prog_->link() ;

    return prog_ ;
}

void PhongMaterialInstance::applyParameters() {

    auto p = material_->prog() ;

    p->setUniform("g_material.ambient", ambient_) ;
    p->setUniform("g_material.diffuse", diffuse_) ;
    p->setUniform("g_material.specular", specular_) ;
    p->setUniform("g_material.shininess", shininess_) ;
}

void PhongMaterialInstance::instantiate() {
    material_ = PhongMaterial::instance(flags_) ;
}

class DiffuseMapMaterial: public Material {
public:
   gl::ShaderProgram::Ptr prog() override ;

    DiffuseMapMaterial(int flags): Material(flags) {}

    static MaterialPtr instance(int flags) {
        static std::map<int, MaterialPtr> s_materials ;
        return materialSingleton<DiffuseMapMaterial>(s_materials, flags) ;

    }

    gl::ShaderProgram::Ptr prog_ ;
};

gl::ShaderProgram::Ptr DiffuseMapMaterial::prog()
{
    if ( prog_ ) return prog_ ;

    gl::Shader::Ptr vs(new gl::Shader(gl::Shader::Vertex)) ;
    std::string preproc ;
    preproc.append("#define HAS_NORMALS\n") ;
    preproc.append("#define HAS_UVs\n") ;
    if ( flags_ & USE_SKINNING ) preproc.append("#define USE_SKINNING\n") ;

    vs->addSourceString(version_header) ;
    vs->addSourceString(preproc) ;
    vs->addSourceString(vertex_shader_code, "vertex_shader_code") ;

    gl::Shader::Ptr fs(new gl::Shader(gl::Shader::Fragment)) ;

    fs->addSourceString(version_header) ;
    fs->addSourceString(phong_fragment_shader_common) ;
    fs->addSourceString(phong_fragment_shader_map, "phong_fragment_shader_map")  ;

    prog_.reset(new gl::ShaderProgram) ;
    prog_->addShader(vs) ;
    prog_->addShader(fs) ;

    prog_->link() ;

    return prog_ ;
}

void DiffuseMapMaterialInstance::applyParameters() {

    auto p = material_->prog() ;

    p->setUniform("g_material.ambient", ambient_) ;
    p->setUniform("g_material.specular", specular_) ;
    p->setUniform("g_material.shininess", shininess_) ;

    p->setUniform("tex_unit", 0) ;

    diffuse_map_.read() ;
    diffuse_map_.upload() ;
}

DiffuseMapMaterialInstance::DiffuseMapMaterialInstance(const Texture2D &tex):
    diffuse_map_(tex) {
}

void DiffuseMapMaterialInstance::instantiate() {
    material_ = DiffuseMapMaterial::instance(flags_) ;
}

class ConstantMaterial: public Material {
public:
    gl::ShaderProgram::Ptr prog() override ;

    ConstantMaterial(int flags): Material(flags_) {}

    static MaterialPtr instance(int flags) {
        static std::map<int, MaterialPtr> s_materials ;
        return materialSingleton<ConstantMaterial>(s_materials, flags) ;
    }

    gl::ShaderProgram::Ptr prog_ ;
};

gl::ShaderProgram::Ptr ConstantMaterial::prog() {

    if ( prog_ ) return prog_ ;

    gl::Shader::Ptr vs(new gl::Shader(gl::Shader::Vertex)) ;

    std::string preproc ;
    if ( flags_ & USE_SKINNING ) preproc.append("#define USE_SKINNING\n") ;

    vs->addSourceString(version_header) ;
    vs->addSourceString(preproc) ;
    vs->addSourceString(vertex_shader_code, "vertex_shader_code") ;

    gl::Shader::Ptr fs(new gl::Shader(gl::Shader::Fragment)) ;

    fs->addSourceString(version_header) ;
    fs->addSourceString(constant_fragment_shader, "constant_fragment_shader") ;

    prog_.reset(new gl::ShaderProgram) ;
    prog_->addShader(vs) ;
    prog_->addShader(fs) ;

    prog_->link() ;

    return prog_ ;
}

ConstantMaterialInstance::ConstantMaterialInstance(const Vector4f &clr):
    clr_(clr) {
}

void ConstantMaterialInstance::instantiate() {
    material_ = ConstantMaterial::instance(flags_) ;
}

void ConstantMaterialInstance::applyParameters() {
    auto p = material_->prog() ;

    p->setUniform("g_material_clr", clr_) ;
}


class PerVertexColorMaterial: public Material {
public:
   gl::ShaderProgram::Ptr prog() override ;

    PerVertexColorMaterial(int flags): Material(flags) {}

    static MaterialPtr instance(int flags) {
        static std::map<int, MaterialPtr> s_materials ;
        return materialSingleton<PerVertexColorMaterial>(s_materials, flags) ;
    }

    gl::ShaderProgram::Ptr prog_ ;
};

gl::ShaderProgram::Ptr PerVertexColorMaterial::prog()
{
    if ( prog_ ) return prog_ ;

    gl::Shader::Ptr vs(new gl::Shader(gl::Shader::Vertex)) ;

    std::string preproc ;
    preproc.append("#define HAS_COLORS\n") ;
    if ( flags_ & USE_SKINNING ) preproc.append("#define USE_SKINNING\n") ;

    vs->addSourceString(version_header) ;
    vs->addSourceString(preproc) ;
    vs->addSourceString(vertex_shader_code, "vertex_shader_code") ;

    gl::Shader::Ptr fs(new gl::Shader(gl::Shader::Fragment)) ;

    fs->addSourceString(version_header) ;
    fs->addSourceString(pervertex_fragment_shader, "pervertex_fragment_shader") ;

    prog_.reset(new gl::ShaderProgram) ;
    prog_->addShader(vs) ;
    prog_->addShader(fs) ;

    prog_->link() ;

    return prog_ ;
}

PerVertexColorMaterialInstance::PerVertexColorMaterialInstance(float op): opacity_(op) {
}

void PerVertexColorMaterialInstance::instantiate() {
    material_ = PerVertexColorMaterial::instance(flags_) ;
}

void PerVertexColorMaterialInstance::applyParameters() {
    auto p = material_->prog() ;

    p->setUniform("opacity", opacity_) ;
}


MaterialInstancePtr Material::makeLambertian(const Eigen::Vector4f &clr) {
    PhongMaterialInstance *p = new PhongMaterialInstance ;

    p->setDiffuse(clr) ;
    p->setSpecular(Vector4f(0, 0, 0, 1)) ;
    p->setShininess(0.0) ;
    return MaterialInstancePtr(p) ;
}

MaterialInstancePtr Material::makeConstant(const Eigen::Vector4f &clr) {
    return  MaterialInstancePtr(new ConstantMaterialInstance(clr)) ;
}

void MaterialInstance::use() {
    if ( !material_ ) instantiate() ;
    material_->prog()->use() ;
}

void MaterialInstance::applyBoneTransform(uint idx, const Matrix4f &tf)
{
    auto p = material_->prog() ;

    ostringstream name ;
    name << "g_bones[" << idx << "]" ;
    p->setUniform(name.str(), tf) ;
}

void MaterialInstance::applyDefaultPerspective(const Matrix4f &cam, const Matrix4f &view, const Matrix4f &model)
{
    Matrix4f mvp =  cam * view * model;
    Matrix4f mv =   view * model;

    Matrix3f wpi = mv.block<3, 3>(0, 0).transpose().eval() ;
    Matrix3f wp(wpi.inverse().eval()) ;

    auto p = material_->prog() ;

    p->setUniform("mvp", mvp) ;
    p->setUniform("mv", mv) ;
    p->setUniform("mvn", wp) ;
    p->setUniform("model", model) ;
}

void MaterialInstance::applyDefaultShadow(const Matrix4f &ls, float bias) {
    auto prog = material_->prog() ;

    Matrix4f bmat ;

    prog->setUniform("mls", ls) ;
    prog->setUniform("depthTexture", 1) ;
    prog->setUniform("shadowBias", bias) ;
}


void MaterialInstance::applyDefaultLight(uint light_index, const LightPtr &light, const Affine3f &tf)
{
    auto prog = material_->prog() ;

    string vname = cvx::util::format("g_light_source[{}]", light_index ++) ;

    if ( const auto &alight = std::dynamic_pointer_cast<AmbientLight>(light) ) {

        prog->setUniform(vname + ".light_type", 0) ;
        prog->setUniform(vname + ".color", alight->color_) ;
    }
    else if ( const auto &dlight = std::dynamic_pointer_cast<DirectionalLight>(light) ) {
        prog->setUniform(vname + ".light_type", 1) ;
        prog->setUniform(vname + ".color", dlight->diffuse_color_) ;
        prog->setUniform(vname + ".direction", tf * dlight->direction_) ;
    }
    else if ( const auto &slight = std::dynamic_pointer_cast<SpotLight>(light) ) {

        prog->setUniform(vname + ".light_type", 2) ;
        prog->setUniform(vname + ".color", slight->diffuse_color_) ;
        prog->setUniform(vname + ".direction", tf * slight->direction_) ;
        prog->setUniform(vname + ".position", tf * slight->position_) ;
        prog->setUniform(vname + ".constant_attenuation", slight->constant_attenuation_) ;
        prog->setUniform(vname + ".linear_attenuation", slight->linear_attenuation_) ;
        prog->setUniform(vname + ".quadratic_attenuation", slight->quadratic_attenuation_) ;
        prog->setUniform(vname + ".spot_exponent", slight->falloff_exponent_) ;
        prog->setUniform(vname + ".spot_cos_cutoff", (float)cos(M_PI*slight->falloff_angle_/180.0)) ;
    }
    else if ( const auto &plight = std::dynamic_pointer_cast<PointLight>(light)) {
        prog->setUniform(vname + ".light_type", 3) ;
        prog->setUniform(vname + ".color", plight->diffuse_color_) ;
        prog->setUniform(vname + ".position", tf * plight->position_) ;
        prog->setUniform(vname + ".constant_attenuation", plight->constant_attenuation_) ;
        prog->setUniform(vname + ".linear_attenuation", plight->linear_attenuation_) ;
        prog->setUniform(vname + ".quadratic_attenuation", plight->quadratic_attenuation_) ;
    }

}



}}
