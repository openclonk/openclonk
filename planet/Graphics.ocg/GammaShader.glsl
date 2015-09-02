
// Gamma uniforms
uniform vec3 gamma;

slice(finish+10) {
    color = vec4(pow(color.rgb, gamma), color.a);
}
