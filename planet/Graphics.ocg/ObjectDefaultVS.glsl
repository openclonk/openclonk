varying vec3 normalDir;

slice(position)
{
	gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;
}

slice(texcoord)
{
	texcoord = gl_MultiTexCoord0.st;
}

slice(normal)
{
	normalDir = normalize(gl_NormalMatrix * gl_Normal);
}
