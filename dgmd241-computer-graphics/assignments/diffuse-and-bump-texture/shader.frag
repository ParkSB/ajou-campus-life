#version 150 core

in vec3 normal;
in vec3 worldPosition;
in vec2 texCoords;

uniform vec4 color;
uniform vec3 lightPosition;
uniform vec3 lightColor;
uniform float shininess;
uniform vec3 cameraPosition;
uniform vec3 ambientLight;
uniform sampler2D diffTex;
uniform sampler2D bumpTex;

out vec4 out_Color;

mat3 getTBN(vec3 N) {
  vec3 Q1 = dFdx(worldPosition), Q2 = dFdy(worldPosition);
  vec2 st1 = dFdx(texCoords), st2 = dFdy(texCoords);
  float D = st1.s * st2.t - st2.s * st1.t;
  return mat3(normalize((Q1 * st2.t - Q2 * st1.t) * D),
      normalize((-Q1 * st2.s + Q2 * st1.s) * D), N);
}

void main(void) {
  vec3 l = lightPosition - worldPosition;
  vec3 L = normalize(l);
  vec3 N = normalize(normal);

  mat3 tbn = getTBN(N);
  float dBdu = texture(bumpTex, texCoords + vec2(0.00001, 0)).r -
    texture(bumpTex, texCoords - vec2(0.00001, 0)).r;
  float dBdv = texture(bumpTex, texCoords + vec2(0, 0.00001)).r -
    texture(bumpTex, texCoords - vec2(0, 0.00001)).r;
  N = normalize(N - dBdu * tbn[0] * 100 - dBdv * tbn[1] * 100);

  vec3 V = normalize(cameraPosition - worldPosition);
  vec3 R = 2 * dot(L, N) * N - L;
  vec3 I = lightColor / dot(l, l);
  vec4 diffColor = texture(diffTex, texCoords);
  vec3 c = diffColor.rgb * max(0, dot(L, N)) * I + diffColor.rgb * ambientLight;
  c += pow(max(0, dot(R, V)), shininess) * I;

  out_Color = vec4(pow(c, vec3(1 / 2.2)), 1);
}

