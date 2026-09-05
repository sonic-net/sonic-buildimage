#ifndef __FGPA_GPIO_H__
#define __FGPA_GPIO_H__

int gpiodev_init(struct pci_dev *dev, struct fpga_dev *fpga);
void gpiodev_exit(struct pci_dev *dev);

#endif /* __FGPA_GPIO_H__ */