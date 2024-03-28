#version 330 core
out vec4 FragColor;

struct PointLight {
    vec3 position;

    vec3 specular;
    vec3 diffuse;
    vec3 ambient;

    float constant;
    float linear;
    float quadratic;
};

struct DirLight {
    vec3 direction;

    vec3 specular;
    vec3 diffuse;
    vec3 ambient;
};

struct Material {
    sampler2D texture_diffuse1;
    sampler2D texture_specular1;
    sampler2D texture_normal1;
    float shininess;
};
in vec2 TexCoords;
in vec3 Normal;
in vec3 FragPos;
in mat3 TBN;
in vec3 TangentLightPos;
in vec3 TangentFragPos;
in vec3 TangentViewPos;

uniform PointLight pointLight;
uniform DirLight dirLight;
uniform Material material;

uniform vec3 viewPosition;
uniform vec3 lightPos;

// calculates the color when using a point light.
vec3 CalcPointLight(PointLight light, vec3 normal, vec3 fragPos, vec3 viewDir)
{
vec3 lightDir = normalize(TangentLightPos - TangentFragPos);    // diffuse shading
    float diff = max(dot(normal, lightDir), 0.0);
    // specular shading
    vec3 halfwayDir = normalize(lightDir + normal);
    float spec = pow(max(dot(viewDir, halfwayDir), 0.0), material.shininess);

    // attenuation
    float distance = length(TangentLightPos - TangentFragPos);     float attenuation = 1.0 / (light.constant + light.linear * distance + light.quadratic * (distance * distance));

    // combine results
    vec3 ambient = light.ambient * vec3(texture(material.texture_diffuse1, TexCoords));
    vec3 diffuse = light.diffuse * diff * vec3(texture(material.texture_diffuse1, TexCoords));
    vec3 specular = light.specular * spec * vec3(texture(material.texture_specular1, TexCoords).xxx);
    ambient *= attenuation;
    diffuse *= attenuation;
    specular *= attenuation;
    return (ambient + diffuse + specular);
}

vec3 CalcDirLight(DirLight light, vec3 normal, vec3 viewDir)
{
   vec3 lightDir = normalize(-light.direction);
   // diffuse shading
   float diff = max(dot(normal, lightDir), 0.0);
   // specular shading
   vec3 halfwayDir = normalize(lightDir + viewDir);
   float spec = pow(max(dot(normal, halfwayDir), 0.0), material.shininess);

   // combine results
   vec3 ambient = light.ambient * vec3(texture(material.texture_diffuse1, TexCoords));
   vec3 diffuse = light.diffuse * diff * vec3(texture(material.texture_diffuse1, TexCoords));
   vec3 specular = light.specular * spec * vec3(texture(material.texture_specular1, TexCoords).xxx);
   return (ambient + diffuse + specular);
}

void main()
{
    vec3 normal = texture(material.texture_normal1, TexCoords).rgb;
    normal = normalize(normal * 2.0 - 1.0);
    vec3 viewDir = normalize(TangentViewPos - TangentFragPos);
    vec3 result = CalcDirLight(dirLight, normal, viewDir);
    result += CalcPointLight(pointLight, normal, TangentFragPos, viewDir);
    FragColor = vec4(result, 1.0);
}