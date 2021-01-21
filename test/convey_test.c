#include <stdio.h>

#include "convey.h"

#if 0
Main({
 Test({"Integer Tests", {
 	int x = 1; int y = 2;
 	Convey("Addition works", func() {
 		So(y == 2);
 		So(y + x == 3);
 		So(x + y == 3);
 		Convey("Even big numbers", {
 			y = 100;
 			So(x + y == 101);
 		});
 		Convey("Notice y is still 2 in this context", {
 			So(y == 2);
 		});
		});
 });
})
#endif

TestMain("Integer Tests", {
    int x = 1;
    int y = 2;
    Convey("Addition works", {
        So(y == 2);
    });
});
