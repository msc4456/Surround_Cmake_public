/*
车模型绘制片段着色器
*/
#version 330 core
precision mediump float;
uniform sampler2D  sTexture;
varying float  LightIntensity;
varying vec2   TexCoord;
void main()
{
    gl_FragColor.rgb = texture2D(sTexture, TexCoord).bbb;
    gl_FragColor.a = 1.0;
}