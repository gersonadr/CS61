#pragma once

struct coordinate {
    float longitude;
    float latitude;
};
typedef struct coordinate coordinate_t;

struct request {
    unsigned long customer_id;
    unsigned long timestamp;
    coordinate_t origin;
    coordinate_t destination;
};
typedef struct request request_t;

void drive(request_t *req);
