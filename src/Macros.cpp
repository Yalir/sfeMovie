
#include "Macros.hpp"

namespace sfe
{
    std::string ff_err2str(int code)
    {
        char buf[AV_ERROR_MAX_STRING_SIZE];
        memset(buf, 0, AV_ERROR_MAX_STRING_SIZE);
        
        av_make_error_string(buf, AV_ERROR_MAX_STRING_SIZE, code);
        return std::string(buf);
    }
}
