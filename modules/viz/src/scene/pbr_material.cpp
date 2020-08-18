#include <cvx/viz/scene/material.hpp>
#include <cvx/util/misc/format.hpp>
#include <Eigen/Core>
#include <Eigen/Geometry>

#include <GL/gl3w.h>

using namespace std ;
using namespace Eigen ;
using namespace cvx::util ;

extern string vertex_shader_code ;

static string pbr_fragment_shader =
        R"(
        precision mediump float;
        in vec3 normal;
        in vec3 position;

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
        float roughness;
        float metallic;
        vec3 albedo;
        };

        uniform MaterialParameters g_material;

        out vec4 FragColor;

        const float PI = 3.14159265359;
        // ----------------------------------------------------------------------------
        float DistributionGGX(vec3 N, vec3 H, float roughness)
        {
            float a = roughness*roughness;
            float a2 = a*a;
            float NdotH = max(dot(N, H), 0.0);
            float NdotH2 = NdotH*NdotH;

            float nom   = a2;
            float denom = (NdotH2 * (a2 - 1.0) + 1.0);
            denom = PI * denom * denom;

            return nom / max(denom, 0.001); // prevent divide by zero for roughness=0.0 and NdotH=1.0
        }
        // ----------------------------------------------------------------------------
        float GeometrySchlickGGX(float NdotV, float roughness)
        {
            float r = (roughness + 1.0);
            float k = (r*r) / 8.0;

            float nom   = NdotV;
            float denom = NdotV * (1.0 - k) + k;

            return nom / denom;
        }
        // ----------------------------------------------------------------------------
        float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness)
        {
            float NdotV = max(dot(N, V), 0.0);
            float NdotL = max(dot(N, L), 0.0);
            float ggx2 = GeometrySchlickGGX(NdotV, roughness);
            float ggx1 = GeometrySchlickGGX(NdotL, roughness);

            return ggx1 * ggx2;
        }
        // ----------------------------------------------------------------------------
        vec3 fresnelSchlick(float cosTheta, vec3 F0)
        {
            return F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0);
        }
        // ----------------------------------------------------------------------------
        void main()
        {
            vec3 N = normalize(normal);
            vec3 V = normalize(-position);

            // calculate reflectance at normal incidence; if dia-electric (like plastic) use F0
            // of 0.04 and if it's a metal, use the albedo color as F0 (metallic workflow)
            vec3 F0 = vec3(0.04);
            F0 = mix(F0, g_material.albedo, g_material.metallic);

            // reflectance equation
            vec3 Lo = vec3(0.0);
            for(int i = 0; i < MAX_LIGHTS; ++i)
            {
                vec3 L ;
                float att = 1.0 ;

                if ( g_light_source[i].light_type == -1 ) continue ;
                else if ( g_light_source[i].light_type == AMBIENT_LIGHT ) {
                    Lo += g_light_source[i].color * g_material.albedo ;
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

                // calculate per-light radiance
                vec3 H = normalize(V + L);

                vec3 radiance = g_light_source[i].color * att;

                // Cook-Torrance BRDF
                float NDF = DistributionGGX(N, H, g_material.roughness);
                float G   = GeometrySmith(N, V, L, g_material.roughness);
                vec3 F    = fresnelSchlick(clamp(dot(H, V), 0.0, 1.0), F0);

                vec3 nominator    = NDF * G * F;
                float denominator = 4 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0);
                vec3 specular = nominator / max(denominator, 0.001); // prevent divide by zero for NdotV=0.0 or NdotL=0.0

                // kS is equal to Fresnel
                vec3 kS = F;
                // for energy conservation, the diffuse and specular light can't
                // be above 1.0 (unless the surface emits light); to preserve this
                // relationship the diffuse component (kD) should equal 1.0 - kS.
                vec3 kD = vec3(1.0) - kS;
                // multiply kD by the inverse metalness such that only non-metals
                // have diffuse lighting, or a linear blend if partly metal (pure metals
                // have no diffuse light).
                kD *= 1.0 - g_material.metallic;

                // scale light by NdotL
                float NdotL = max(dot(N, L), 0.0);

                // add to outgoing radiance Lo
                Lo += (kD * g_material.albedo / PI + specular) * radiance * NdotL;  // note that we already multiplied the BRDF by the Fresnel (kS) so we won't multiply by kS again
            }

            // ambient lighting (note that the next IBL tutorial will replace
            // this ambient lighting with environment lighting).
            vec3 ambient = vec3(0.03) * g_material.albedo * 1/*ao*/;

            vec3 color = ambient + Lo;

            // HDR tonemapping
            color = color / (color + vec3(1.0));
            // gamma correct
            color = pow(color, vec3(1.0/2.2));

            FragColor = vec4(color, 1.0);
        }


        )";



namespace cvx { namespace viz {

using MaterialPtr = std::shared_ptr<Material> ;


class PBRMaterial: public Material {
public:

    PBRMaterial(int flags): Material(flags) {}

    gl::ShaderProgram::Ptr prog() override ;

    static MaterialPtr instance(int flags) {
        static std::map<int, MaterialPtr> s_materials ;
        return materialSingleton<PBRMaterial>(s_materials, flags) ;
    }

    gl::ShaderProgram::Ptr prog_ ;
};

static const char *version_header = "#version 330\n" ;

gl::ShaderProgram::Ptr PBRMaterial::prog()
{
    if ( prog_ ) return prog_ ;

    gl::Shader::Ptr vs(new gl::Shader(gl::Shader::Vertex)) ;

    std::string preproc ;
    preproc.append("#define HAS_NORMALS\n") ;
    if ( flags_ & USE_SKINNING )  preproc.append("#define USE_SKINNING\n") ;

    vs->addSourceString(version_header) ;
    vs->addSourceString(preproc) ;
    vs->addSourceString(vertex_shader_code, "vertex_shader_code") ;

    gl::Shader::Ptr fs(new gl::Shader(gl::Shader::Fragment)) ;

    fs->addSourceString(version_header) ;
    fs->addSourceString(pbr_fragment_shader) ;

    prog_.reset(new gl::ShaderProgram) ;
    prog_->addShader(vs) ;
    prog_->addShader(fs) ;

    prog_->link() ;

    return prog_ ;
}

void PBRMaterialInstance::applyParameters() {

    auto p = material_->prog() ;

    p->setUniform("g_material.albedo", albedo_) ;
    p->setUniform("g_material.metallic", metallic_) ;
    p->setUniform("g_material.roughness", roughness_) ;
}

void PBRMaterialInstance::instantiate() {
    material_ = PBRMaterial::instance(flags_) ;
}


}}
