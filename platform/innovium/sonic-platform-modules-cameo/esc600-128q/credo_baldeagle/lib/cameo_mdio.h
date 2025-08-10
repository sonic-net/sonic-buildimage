

#ifndef __CAMEOMDIO_H__
#define __CAMEOMDIO_H__

void ForDelay();
void mdio_write(int phyad, int devad, int offset_in_mmd, int data);
unsigned short mdio_read(int phyad, int devad, int offset_in_mmd);
int lscpcie_open();
uint32_t lscpcie_close();

#endif /* __CAMEOMDIO_H__ */
