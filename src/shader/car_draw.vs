	/*
    车模型绘制顶点着色器
    */
    #version 330 core
    attribute vec3 inVertex;
	attribute vec3 inNormal;
	attribute vec2 inTexCoord;
	uniform mat4  MVPMatrix;
	uniform vec3  LightDirection;
	varying float  LightIntensity;
	varying vec2   TexCoord;
	void main()
	{
        gl_Position = MVPMatrix * vec4(inVertex, 1.0);
        TexCoord = inTexCoord;
        LightIntensity = dot(inNormal, -LightDirection);
    };
