#pragma once

/**
 * A bunch of helper code to make everything work.  You shouldn't need to change this
 *   code, but you may need to look at some of the method declarations to get an idea
 *   of the API that driver_thread is using.
 */

#include <stdlib.h>
#include <stdbool.h>
#include <pthread.h>

/**
 * Tracking code and structures
 */
// per-meal/customer tracking
struct private_tracking;

struct private_tracking *private_tracking_create();
void private_tracking_destroy(struct private_tracking *stats);

int num_picked_up(const struct private_tracking *);
int num_completed(const struct private_tracking *);

void pick_up(struct private_tracking *);
void complete(struct private_tracking *);

// overall app tracking
struct ubler_stats
{
    int total_meals_picked_up;
    int total_customers_picked_up;

    int total_meals_ignored;
    int total_customers_ignored;

    int total_meals_overserviced;
    int total_customers_overserviced;

    int driver_meals_picked_up;
    int driver_customers_picked_up;

    int driver_meals_delivered;
    int driver_customers_delivered;
};

// tuning ubler parameters
struct ubler_params
{
    // set in ubler.conf
    int sleepTime;
    bool deliverMealsAndCustomers;

    // passed in from command line
    bool mixUpMeals;
};

/**
 * Restaurant methods
 */
struct restaurant
{
    struct location *location;

    // add other things as you go along
};

// init/cleanup restaurant
void init_restaurant(struct restaurant *restaurant);
void cleanup_restaurant(struct restaurant *restaurant);

// get/set location; get puts it in *ret
void restaurant_get_location(const struct restaurant *restaurant, struct location **ret);
void restaurant_set_location(struct restaurant *restaurant, const struct location *location);

/**
 * customer and meal placeholder structs
 */

struct customer;
struct meal;

/**
 * Customer methods
 * Customer struct and init/destroy defined in ubler.h
 */

// get/set location; get puts it in *ret
void customer_get_location(const struct customer *customer, struct location **ret);
void customer_set_location(struct customer *customer, const struct location *location);

// get/set customer's meal; get puts it in *ret
void customer_get_meal(const struct customer *customer, struct meal **ret);
void customer_set_meal(struct customer *customer, const struct meal *meal);

// returns number of times this customer has been picked up
int customer_picked_up(const struct customer *customer);
// returns number of times this customer's request has been completed
int customer_completed(const struct customer *customer);

// pick this customer up
void pick_customer_up(struct customer *customer);
// drop off customer at restaurant
void drop_off_customer(struct customer *customer, struct restaurant *restaurant);

/**
 * Meal methods
 * Meal struct and init/destroy defined in ubler.h
 */

// get/set restaurant; get puts it in *ret
void meal_get_restaurant(const struct meal *meal, struct restaurant **ret);
void meal_set_restaurant(struct meal *meal, const struct restaurant *restaurant);

// get/set meal's customer; get puts it in *ret
void meal_get_customer(const struct meal *meal, struct customer **ret);
void meal_set_customer(struct meal *meal, const struct customer *customer);

// returns number of times this meal has been picked up
int meal_picked_up(const struct meal *meal);
// returns number of times this meal's request has been completed
int meal_completed(const struct meal *meal);

// pick this meal up
void pick_meal_up(struct meal *meal);
// drop off meal at customer
void drop_off_meal(struct meal *meal, struct customer *customer);

/**
 * Driver structs and helpers
 */
struct driver_stats
{
    int num_meals_picked_up;
    int num_customers_picked_up;

    int num_meals_delivered;
    int num_customers_delivered;
};

struct driver {
    struct ubler_params *params; // parameters for problem tuning
    struct driver_stats *stats; // keeps track about driver stats

    struct location *location; // where the driver is
};

void driver_pick_meal_up(struct driver *driver, struct meal *meal);
void driver_pick_customer_up(struct driver *driver, struct customer *customer);
void driver_drop_off_meal(struct driver *driver, struct meal *meal, struct customer *customer);
void driver_drop_off_customer(struct driver *driver, struct customer *customer, struct restaurant *restaurant);
void driver_drive_to_location(struct driver *driver, const struct location *location);

/**
 * Location definition
 */
struct location
{
    // this doesn't actually need to do anything except
    //   take up some space
    char dummy;
};

/**
 * Request methods and helpers
 */
// puts either a customer or a meal in *ret
// if we put a customer in *ret, *isCustomer will be set to true
// otherwise, *isCustomer will be set to false
// 
// if ret==NULL or isCustomer==NULL, or we are out of
// meals or customers that need servicing, we will return -1
// 
// we return 0 on success
int receive_request(void **ret, bool *isCustomer);

// start num_requests with num_drivers and ubler_params
// put results of the simulation in stats
int create_and_run_requests(
    const unsigned int num_requests, 
    const unsigned int num_restaurants,
    const unsigned int num_drivers, 
    const struct ubler_params *ubler_params,
    struct ubler_stats *stats);

// swap meal1 and meal2
//   if preserveValidity == true, also swap the customers on the other side,
//   so we maintain a valid two-way customer to meal mapping
void swap_meals(struct meal *meal1, struct meal* meal2, const bool preserveValidity);
void init_restaurants(
    struct restaurant *restaurants, 
    const unsigned int num_restaurants,
    struct location *locations,
    const unsigned int num_locations);
void cleanup_restaurants(
    struct restaurant *restaurants, 
    const unsigned int num_restaurants);
void init_requests(
    struct meal *meals,
    struct customer *customers,
    const unsigned int num_requests,
    struct restaurant *restaurants,
    const unsigned int num_restaurants,
    struct location *locations,
    const unsigned int num_locations);
void mix_up_meals(
    struct meal *meals,
    const unsigned int num_requests);
void cleanup_requests(
    struct meal *meals,
    struct customer *customers,
    const unsigned int num_requests);
void dispatch_drivers(
    const struct location *driverHub,
    const struct ubler_params *ubler_params,
    struct driver_stats *driver_stats,
    struct driver *drivers,
    pthread_t *driver_threads,
    const unsigned int num_drivers);
void record_stats(
    const struct meal *meals,
    const struct customer *customers,
    const unsigned int num_requests,
    const struct driver_stats *driver_stats,
    const unsigned int num_drivers,
    struct ubler_stats *ret_stats);

