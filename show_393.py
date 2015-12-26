# -*- coding:utf-8 -*-

import gevent
import serial
import numpy as np

from traits.api import HasTraits, Str, Bool, List, Instance, Button, on_trait_change
from traitsui.api import View, Item, HGroup, VGroup, StatusItem, Handler
from chaco.api import Plot, VPlotContainer, ArrayPlotData

from enable.component_editor import ComponentEditor
from enable.api import Component

HEIGHT = 1
WIDTH = 0.8
PORT = '/dev/cu.usbmodem1411'
BAUDRATE = 115200
STEP = 8
class CloseHandler(Handler):
    def closed(self, info, is_ok):
        info.object.closed = True
        return True

class gauss_data():

    def __init__(self,x, y, z):
        self.x = x
        self.y = y
        self.z = z

def _creat_plot(datas):
    x = [gauss.x for gauss in datas[-100:]]
    y = [gauss.y for gauss in datas[-100:]]
    z = [gauss.z for gauss in datas[-100:]]
    i = range(len(x))
    p = Plot(ArrayPlotData(i=i, x=x, y=y, z=z), padding=10, origin = "top left")
    # p.origin_axis_visible = True

    p.border_visible = True
    p.legend.visible = True
    p.x_grid.visible = False
    # p.y_grid.visible = False
    p.padding_left = 35
    # p.value_range.low = -50
    # p.value_range.high = 50
    p.plot(('i', 'x'), type="line", line_width=2, color="red", name = 'X')
    p.plot(('i', 'y'), type="line", line_width=2, color="blue", name = 'Y')
    p.plot(('i', 'z'), type="line", line_width=2, color="green", name = 'Z')

    return p

class Mlx393(HasTraits):
    """Mlx90393 show data tool"""

    chip_0 = []
    chip_1 = []
    chip_2 = []
    chip_3 = []

    glet = Instance(gevent.Greenlet)
    ser = serial.Serial(PORT, BAUDRATE)
    btn_start = Button(u'启动/停止')
    btn_save = Button(u'保存数据')
    plot_show = Instance(VPlotContainer)
    # plot_show_0 = Instance(VPlotContainer)
    # plot_show = Instance(Component)
    status_info = Str('MLX90393')

    closed = Bool(False)

    view = View(
        VGroup(
            Item('plot_show', editor=ComponentEditor()),
            Item('btn_start', show_label=False),
            Item('btn_save', show_label=False),
            show_labels=False, show_border = True
            ),
            # VGroup(
            #     Item('plot_show_0', editor=ComponentEditor()),
            #     show_labels=False, show_border = True
            #     ),
        resizable=True,
        height=HEIGHT,
        width=WIDTH,
        statusbar=StatusItem('status_info', width=0.8),
        title='MLX90393'
    )

    def __init__(self):
        super(Mlx393, self).__init__()
        self.draw_plot()

    def draw_plot(self):
        # x=np.linspace(3, -3, 100)
        # y=np.sin(x)
        # plotdata = ArrayPlotData(x=x, y=y)
        # p_1 = Plot(plotdata, padding=10)
        # p_1.plot(('x', 'y'), type="line", color="red")
        #
        #
        # p_2 = Plot(plotdata, padding=10)
        # p_2.plot(('x','y'), type="line", color="blue")

        container = VPlotContainer(bgcolor="silver")
        for i in [self.chip_0, self.chip_1, self.chip_2]:
            container.add(_creat_plot(i))
        # self.plot_show_0 = VPlotContainer(_creat_plot(self.chip_0))
        self.plot_show = container
        # self.plot_show = VPlotContainer(p_2, p_1)

    def get_data(self):
        text = ''
        flag = 0
        while True:
            text = self.ser.read(1)
            if text:
                text = text + self.ser.readline()
                if text.endswith('e\r\n') and text.startswith('b'):
                    self.status_info = text
                    text = text.replace('b', '').replace('e', '').replace('\r\n', '').replace('\r', '').split('%')
                    try:
                        self.chip_0.append(gauss_data(*map(float, text[:3])))
                        self.chip_1.append(gauss_data(*map(float, text[3:-3])))
                        self.chip_2.append(gauss_data(*map(float, text[-3:])))
                        flag += 1
                        if flag == STEP:
                            self.draw_plot()
                            flag = 0
                    except:
                        pass
                        # self.chip_0 = self.chip_0[-100:]
                        # self.chip_1 = self.chip_1[-100:]
                        # self.chip_2 = self.chip_2[-100:]
                        # print "rx-error"
            text = ''
            gevent.sleep(0.001)

    def _btn_start_fired(self):
        if not self.glet:
            self.glet = gevent.spawn(self.get_data)
        else:
            self.glet.kill()
            self.glet = None

    def _btn_save_fired(self):
        try:
            self.glet.kill()
            self.glet = None
        except:
            pass
        np.savetxt("data_0.csv", [(gauss.x, gauss.y, gauss.z) for gauss in self.chip_0], fmt="%f", delimiter=",")
        np.savetxt("data_1.csv", [(gauss.x, gauss.y, gauss.z) for gauss in self.chip_1], fmt="%f", delimiter=",")
        np.savetxt("data_2.csv", [(gauss.x, gauss.y, gauss.z) for gauss in self.chip_2], fmt="%f", delimiter=",")
        self.status_info = u"已将数据保存到data_x.csv!保存数据成功！"
if __name__ == "__main__":
    mlx90393 = Mlx393()
    # mlx90393.configure_traits()
    mlx90393.edit_traits(handler=CloseHandler())
    from pyface.api import GUI
    gui = GUI()
    while not mlx90393.closed:
        gui.process_events()
        gevent.sleep(0.001)
