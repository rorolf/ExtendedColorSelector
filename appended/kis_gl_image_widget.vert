#########################
## kis_gl_image_widget.vert
#########################




/*
 *  SPDX-FileCopyrightText: 2019 Dmitry Kazakov <dimula73@gmail.com>
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef USE_OPENGLES
#define INATTR in
#define OUTATTR out
#define highp
#else
#define INATTR attribute
#define OUTATTR varying
#endif
uniform mat4 viewProjectionMatrix;
INATTR highp vec3 vertexPosition;
INATTR highp vec2 texturePosition;
OUTATTR highp vec4 textureCoordinates;
void main()
{
   textureCoordinates = vec4(texturePosition.x, texturePosition.y, 0.0, 1.0);
   gl_Position = viewProjectionMatrix * vec4(vertexPosition.x, vertexPosition.y, 0.0, 1.0);
}





#########################
## kis_gl_image_widget.qrc
#########################




<!--
  SPDX-FileCopyrightText: none
  SPDX-License-Identifier: CC0-1.0
-->
<RCC>
    <qresource prefix="/">
        <file>kis_gl_image_widget.frag</file>
        <file>kis_gl_image_widget.vert</file>
    </qresource>
</RCC>





#########################
## kis_gl_image_widget.frag
#########################




/*
 *  SPDX-FileCopyrightText: 2019 Dmitry Kazakov <dimula73@gmail.com>
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef USE_OPENGLES
#define INATTR in
#define OUTATTR out
#define DECLARE_OUT_VAR out vec4 f_fragColor;
#define OUT_VAR f_fragColor
#define highp
#define texture2D texture
#else
#define INATTR varying
#define DECLARE_OUT_VAR
#define OUT_VAR gl_FragColor
#endif
// vertices data
INATTR highp vec4 textureCoordinates;
uniform sampler2D f_tileTexture;
DECLARE_OUT_VAR

void main()
{
    // get the fragment color from the tile texture
    highp vec4 color = texture2D(f_tileTexture, textureCoordinates.st);
    OUT_VAR = vec4(color);  
}





