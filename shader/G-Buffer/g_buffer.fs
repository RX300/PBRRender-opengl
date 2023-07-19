#version 460 core
layout (location = 0) out vec4 gPositionRoughness;
layout (location = 1) out vec4 gNormalAO;
layout (location = 2) out vec4 gAlbedoMetallic;
// layout (location = 3) out vec3 gEmission;

in vec2 TexCoords;
in vec3 WorldPos;
in vec3 Normal;

//material parameters block
layout (std140,binding=0) uniform MaterialBlock
{
 vec3 albedo;
 float metallic;
 float roughness;

 bool useAlbedoMap;
 bool useNormalMap;
 bool useMetallicMap;
 bool useRoughnessMap;
 bool useAOMap;
 bool useEmissiveMap;
}materialProperties;
// texture samplers struct(不透明数据只能放在uniform里)
struct MaterialTexture{
 sampler2D albedoMap;
 sampler2D normalMap;
 sampler2D metallicMap;
 sampler2D roughnessMap;
 sampler2D aoMap;
};
uniform MaterialTexture material;

vec3 getNormalFromMap()
{
    vec3 tangentNormal = texture(material.normalMap, TexCoords).xyz * 2.0 - 1.0;

    vec3 Q1  = dFdx(WorldPos);
    vec3 Q2  = dFdy(WorldPos);
    vec2 st1 = dFdx(TexCoords);
    vec2 st2 = dFdy(TexCoords);

    vec3 N   = normalize(Normal);
    vec3 T  = normalize(Q1*st2.t - Q2*st1.t);
    vec3 B  = -normalize(cross(N, T));
    mat3 TBN = mat3(T, B, N);

    return normalize(TBN * tangentNormal);
}

void main()
{    
    vec3 albedo = materialProperties.useAlbedoMap? (texture(material.albedoMap, TexCoords).rgb) : materialProperties.albedo;
    float metallic= materialProperties.useMetallicMap? (texture(material.metallicMap, TexCoords).b) : materialProperties.metallic;
    float roughness = materialProperties.useRoughnessMap? (texture(material.roughnessMap, TexCoords).g) : materialProperties.roughness;
    //float ao = material.useAOMap? (texture(material.aoMap, TexCoords).r) : 1.0f;
    float ao=1.0f;
    
    // input lighting data
    vec3 N=materialProperties.useNormalMap? getNormalFromMap():Normal;
    //向gPositionRoughness输出的是世界坐标和粗糙度
    // store the fragment position vector in the first gbuffer texture
    gPositionRoughness.xyz = WorldPos;
    // also store the per-fragment roughness in the gbuffer
    gPositionRoughness.w = roughness;

    //向gNormalAO输出的是法线和AO
    // store the fragment normal vector in the second gbuffer texture
    gNormalAO.xyz = N;
    // also store the per-fragment ambient occlusion in the gbuffer
    gNormalAO.w = ao;

    //向gAlbedoMetallic输出的是漫反射和镜面反射
    // store the fragment color vector in the third gbuffer texture
    gAlbedoMetallic.rgb = albedo;
    // store specular intensity in gAlbedoSpec's alpha component
    gAlbedoMetallic.a = metallic;
}