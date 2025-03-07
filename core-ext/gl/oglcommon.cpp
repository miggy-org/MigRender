#include "defines.h"
#include "matrix.h"
#include "migexcept.h"

#include "oglcommon.h"

using namespace MigRender;

void CheckGLError(const char* szError)
{
    GLenum err = glGetError();
    if (err != GL_NO_ERROR)
    {
        const GLubyte* errStr = gluErrorString(err);
        throw mig_exception(szError);
    }
}

double ConvertToRadians(double degrees)
{
    return (degrees * M_PI / 180.0);
}

const GLfloat* ConvertMatrixToFloat(const CMatrix& tm)
{
    // OpenGL is column centric, versus MigRender which is row centric
    static GLfloat vals[16];
    //GLfloat* vals = new GLfloat[16];
    //for (int i = 0; i < 16; i++)
    //    vals[i] = static_cast<float>(tm.GetCTM()[i]);
    vals[0] = static_cast<float>(tm.GetCTM()[0]);
    vals[1] = static_cast<float>(tm.GetCTM()[4]);
    vals[2] = static_cast<float>(tm.GetCTM()[8]);
    vals[3] = static_cast<float>(tm.GetCTM()[12]);
    vals[4] = static_cast<float>(tm.GetCTM()[1]);
    vals[5] = static_cast<float>(tm.GetCTM()[5]);
    vals[6] = static_cast<float>(tm.GetCTM()[9]);
    vals[7] = static_cast<float>(tm.GetCTM()[13]);
    vals[8] = static_cast<float>(tm.GetCTM()[2]);
    vals[9] = static_cast<float>(tm.GetCTM()[6]);
    vals[10] = static_cast<float>(tm.GetCTM()[10]);
    vals[11] = static_cast<float>(tm.GetCTM()[14]);
    vals[12] = static_cast<float>(tm.GetCTM()[3]);
    vals[13] = static_cast<float>(tm.GetCTM()[7]);
    vals[14] = static_cast<float>(tm.GetCTM()[11]);
    vals[15] = static_cast<float>(tm.GetCTM()[15]);
    return vals;
}

GLuint LoadTextureMap(const CImageBuffer* pbuf)
{
    GLuint textureId = 0;

    if (pbuf != NULL)
    {
        GLenum format;
        switch (pbuf->GetFormat())
        {
        case ImageFormat::RGB:  format = GL_RGB; break;
        case ImageFormat::BGRA: format = GL_BGRA; break;
        case ImageFormat::BGR:  format = GL_BGR; break;
        case ImageFormat::Bump: format = GL_RG; break;
        case ImageFormat::GreyScale: format = GL_RED; break;
        default: format = GL_RGBA; break;
        }
        int w, h;
        pbuf->GetSize(w, h);

        glGenTextures(1, &textureId);

        glBindTexture(GL_TEXTURE_2D, textureId);
        glTexImage2D(GL_TEXTURE_2D, 0, format, w, h, 0, format, GL_UNSIGNED_BYTE, pbuf->GetImageData().data());
        CheckGLError("Texture load error");
        glGenerateMipmap(GL_TEXTURE_2D);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    }

    return textureId;
}
