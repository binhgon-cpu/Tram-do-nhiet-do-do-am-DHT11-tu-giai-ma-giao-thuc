#ifndef INC_DHT11_H_
#define INC_DHT11_H_

#include "main.h" // chua thu vien Hal va dinh nghia chan GPIO

/* Ham doc cam bien DHT11
 * Tra ve:
 *   0: doc thanh cong
 *   1: Loi Timeout (Rut cam bien / Mat ket noi)
 *   2: Loi Checksum (Du lieu bi sai lech)
 */
uint8_t DHT11_Read(uint8_t *nhiet_do, uint8_t *do_am);

#endif /* INC_DHT11_H_ */
