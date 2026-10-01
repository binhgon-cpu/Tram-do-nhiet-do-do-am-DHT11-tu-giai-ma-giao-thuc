#ifndef INC_DHT11_H_
#define INC_DHT11_H_

#include "main.h"
/*
 * Hàm đọc dữ liệu DHT11
 * Tham số: Truyền vào địa chỉ của 2 biến nhiệt độ và độ ẩm
 * Trả về:
 *   0: Thành công
 *   1: Lỗi Timeout (Bị rút dây, không phản hồi)
 *   2: Lỗi Checksum (Nhiễu dữ liệu)
 */
uint8_t DHT11_Read(uint8_t *nhiet_do, uint8_t *do_am);

#endif /* INC_DHT11_H_ */
