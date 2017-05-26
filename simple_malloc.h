//
//  simple_malloc.h
//  SimpleSTL
//
//  Created by LightCong on 17/5/22.
//  Copyright © 2017年 dacuobi. All rights reserved.
//

#ifndef simple_malloc_h
#define simple_malloc_h

#define TRUE 1
#define FALSE 0



void simple_install();
void simple_uninstall();
void *simple_malloc(unsigned int size);
void simple_free(void *ptr);
void *simple_realloc(void *ptr, unsigned new_size);

#endif /* simple_malloc_h */
