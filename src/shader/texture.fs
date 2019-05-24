#version 330 core
out vec4 FragColor;

in vec2 oTexCoord1;   //前通道纹理坐标输入
in vec2 oTexCoord2;   //左通道纹理坐标输入
in vec2 oTexCoord3;   //后通道纹理坐标输入
in vec2 oTexCoord4;   //右通道纹理坐标输入
in float oTexWeight1; //前通道纹理坐标权重输入
in float oTexWeight2; //左通道纹理坐标权重输入
in float oTexWeight3; //后通道纹理坐标权重输入
in float oTexWeight4; //右通道纹理坐标权重输入

// 纹理采样，光栅化之后
uniform sampler2D texture1;//前2D纹理采样
uniform sampler2D texture2;//左2D纹理采样
uniform sampler2D texture3;//后2D纹理采样
uniform sampler2D texture4;//右2D纹理采样

void main()
{
	// linearly interpolate between both textures (80% container, 20% awesomeface)
	// FragColor = mix(texture(texture1, TexCoord), texture(texture2, TexCoord), 0.2);

	FragColor =  texture2D(texture1,oTexCoord1)*oTexWeight1
				+texture2D(texture2,oTexCoord2)*oTexWeight2
				+texture2D(texture3,oTexCoord3)*oTexWeight3
				+texture2D(texture4,oTexCoord4)*oTexWeight4;
	// FragColor = texture(texture1,oTexCoord1)*0.25+texture(texture2,oTexCoord2)*0.25+texture(texture3,oTexCoord3)*0.25+texture(texture4,oTexCoord4)*0.25;
	// FragColor = texture(texture1,oTexCoord1);
	// FragColor = texture(texture2,oTexCoord2);
	// FragColor = texture(texture3,oTexCoord3);
	// FragColor = texture(texture4,oTexCoord1);

	// FragColor = texture(texture1,oTexCoord1)*0.5+ texture(texture2,oTexCoord2)*0.5;
	// FragColor = vec4(1.0,0.0,0.0,0.0);
}