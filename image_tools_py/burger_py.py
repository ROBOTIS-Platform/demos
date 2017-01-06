# Copyright 2017 Open Source Robotics Foundation, Inc.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

# System imports
import base64
import random

# OpenCV
import cv2

# Numpy
import numpy

# THE FOLLOWING IS A BURGER IN BASE64.  RESPECT IT

BURGER_BASE64 = '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'  # noqa


class Burger(object):

    def __init__(self):
        burger_png = base64.b64decode(BURGER_BASE64)
        np_img = numpy.fromstring(burger_png, dtype=numpy.uint8)
        self.burger_template = cv2.imdecode(np_img, cv2.IMREAD_COLOR)

        # We flood fill the burger template with "1,1,1" (BGR) so that we can
        # remove the borders during rendering.
        h, w = self.burger_template.shape[:2]
        mask = numpy.zeros((h + 2, w + 2), numpy.uint8)
        cv2.floodFill(self.burger_template, mask, (1, 1), (1, 1, 1))

        random.seed()

        self.width = 0
        self.height = 0
        self.num_burgers = 0
        self.burger_list = []

    def render_burger(self, width, height):
        # The basic idea here is to render a number of burgers into a OpenCV mat,
        # moving them on each successful iteration.  So when the requested
        # resolution changes (this includes the very first time we are called),
        # we generate a random number of burgers, at random starting locations,
        # moving at random speeds, and store that list of burgers in the object.
        # We then render the burgers onto the mat and return it.  On subsequent
        # render_burger() method calls, we keep the same list of burgers from
        # before, but move them according to their X and Y speed, "bouncing" them
        # off of the side of the mat if they run into it.
        class OneBurger(object):

            def __init__(self, x, y, x_inc, y_inc):
                self.x = x
                self.y = y
                self.x_inc = x_inc
                self.y_inc = y_inc

        if self.width != width or self.height != height:
            num_burgers = random.randrange(2, 10)
            width_max = width - self.burger_template.shape[1] - 1
            height_max = height - self.burger_template.shape[0] - 1
            for b in range(0, num_burgers):
                x = random.randrange(0, width_max)
                y = random.randrange(0, height_max)
                x_inc = random.randrange(1, 3)
                y_inc = random.randrange(1, 3)
                self.burger_list.append(OneBurger(x, y, x_inc, y_inc))
            self.width = width
            self.height = height

        # We want an OpenCV Mat with CV_8UC3, which is 3 channels
        burger_buf = numpy.zeros((height, width, 3), numpy.uint8)

        # TODO(clalancette): This is the slow way to do this, in that we iterate
        # over every pixel by hand, looking for the flood fill and thus whether
        # we should render that pixel.  However, I was not able to figure out the
        # OpenCV python calls to do this in a nicer way, so we leave this for now.
        for b in self.burger_list:
            for y in range(0, self.burger_template.shape[0]):
                for x in range(0, self.burger_template.shape[1]):
                    bitem = self.burger_template.item(y, x, 0)
                    gitem = self.burger_template.item(y, x, 1)
                    ritem = self.burger_template.item(y, x, 2)
                    if bitem != 1 or gitem != 1 or ritem != 1:
                        burger_buf.itemset((b.y + y, b.x + x, 0), bitem)
                        burger_buf.itemset((b.y + y, b.x + x, 1), gitem)
                        burger_buf.itemset((b.y + y, b.x + x, 2), ritem)
            b.x += b.x_inc
            b.y += b.y_inc

            # Bounce if needed
            if b.x < 0 or b.x > (width - self.burger_template.shape[1] - 1):
                b.x_inc *= -1
                b.x += 2 * b.x_inc
            if b.y < 0 or b.y > (height - self.burger_template.shape[0] - 1):
                b.y_inc *= -1
                b.y += 2 * b.y_inc

        return burger_buf
