#include "css.h"

char* generateCss() {
    return("<style>\
            body {\
            width: 100%;\
            height 100%;\
            display: flex;\
            flex-direction: column;\
            gap: 1rem;\
            }\
            a {\
            font-size: 1.5rem;\
            text-decoration: none;\
            }\
            h1 {\
            font-size: 2rem;\
            }\
            </style>");
}
