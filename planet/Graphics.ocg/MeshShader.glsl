varying vec2 texcoord;

slice(init+1)
{
  // TODO: Add emission part of the material. Note we cannot just
  // add this to the color, but it would need to be handled separately,
  // such that it is independent from the incident light direction.
  // Could make it #ifdef MESH.
  color = gl_FrontMaterial.diffuse * color;
}
