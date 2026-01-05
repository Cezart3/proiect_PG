#version 410 core

out vec4 fragColor;

void main()
{
    // gl_FragDepth is updated automatically.
    fragColor = vec4(1.0); // Dummy write
}
