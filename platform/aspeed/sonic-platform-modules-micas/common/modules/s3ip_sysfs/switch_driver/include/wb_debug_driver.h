#ifndef _WB_DEBUG_DRIVER_H_
#define _WB_DEBUG_DRIVER_H_


ssize_t dfd_debug_data_common_attr(wb_main_dev_type_t main_dev_type, wb_minor_dev_type_t minor_dev_type,
    debug_data_dev_class_t dev_class, unsigned int attr_type, unsigned int sensor_index, const char *value);


#endif /* _WB_DBBUG_DRIVER_H_ */
