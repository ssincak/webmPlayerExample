/*
*  Copyright (c) 2016 Stefan Sincak. All Rights Reserved.
*
*  Use of this source code is governed by a BSD-style license
*  that can be found in the LICENSE file in the root of the source
*  tree.
*/

#include "webm_player.hpp"
#include <iostream>

int SDL_main(int argc, char **argv)
{
    wp::WebmPlayer player;

    if (argc < 2)
    {
        std::cout << "Usage: webm_player video_file.webm";
        std::cin.get();
        return 1;
    }

    if (!player.load(argv[1]))
    {
        std::cout << "Failed to initialize player";
        std::cin.get();
    }
    else
    {
        player.run();
    }

    player.destroy();

    return 0;
}
