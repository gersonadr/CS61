/**
 * Implementations of all the helper code.  You really shouldn't have
 * to change any of this.
 */

#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <unistd.h>
#include "ubler.h"
#include "ubler_helpers.h"

/**
 * Tracking code
 */
struct private_tracking
{
    int num_picked_up;
    int num_completed;
};

struct private_tracking *private_tracking_create()
{
    struct private_tracking *tracking = malloc(sizeof(struct private_tracking));
    tracking->num_picked_up = 0;
    tracking->num_completed = 0;
    return tracking;
}

void private_tracking_destroy(struct private_tracking *stats)
{
    free(stats);
}

int num_picked_up(const struct private_tracking *stats)
{
    return stats->num_picked_up;
}

int num_completed(const struct private_tracking *stats)
{
    return stats->num_completed;
}

void pick_up(struct private_tracking *stats)
{
    stats->num_picked_up++;
}

void complete(struct private_tracking *stats)
{
    stats->num_completed++;
}

/**
 * Restaurant code
 */
// init/cleanup restaurant
void init_restaurant(struct restaurant *restaurant)
{
    restaurant->location = NULL;
}
void cleanup_restaurant(struct restaurant *restaurant)
{
    (void) restaurant;

    // do nothing
}

// get/set location; get puts it in *ret
void restaurant_get_location(const struct restaurant *restaurant, struct location **ret)
{
    *ret = restaurant->location;
}
void restaurant_set_location(struct restaurant *restaurant, const struct location *location)
{
    restaurant->location = (struct location *)location;
}

/**
 * Customer code
 */
// get/set location; get puts it in *ret
void customer_get_location(const struct customer *customer, struct location **ret)
{
    *ret = customer->location;
}
void customer_set_location(struct customer *customer, const struct location *location)
{
    customer->location = (struct location *)location;
}

// get/set customer's meal; get puts it in *ret
void customer_get_meal(const struct customer *customer, struct meal **ret)
{
    *ret = customer->meal;
}
void customer_set_meal(struct customer *customer, const struct meal *meal)
{
    customer->meal = (struct meal *)meal;
}

// returns number of times this customer has been picked up
int customer_picked_up(const struct customer *customer)
{
    return num_picked_up(customer->stats);
}
// returns number of times this customer's request has been completed
int customer_completed(const struct customer *customer)
{
    return num_completed(customer->stats);
}

// pick this customer up
void pick_customer_up(struct customer *customer)
{
    pick_up(customer->stats);
}
// drop off customer at restaurant
void drop_off_customer(struct customer *customer, struct restaurant *restaurant)
{
    (void) restaurant;

    complete(customer->stats);
    complete(customer->meal->stats);
}

/**
 * Meal code
 */
// get/set restaurant; get puts it in *ret
void meal_get_restaurant(const struct meal *meal, struct restaurant **ret)
{
    *ret = meal->restaurant;
}
void meal_set_restaurant(struct meal *meal, const struct restaurant *restaurant)
{
    meal->restaurant = (struct restaurant *)restaurant;
}

// get/set meal's customer; get puts it in *ret
void meal_get_customer(const struct meal *meal, struct customer **ret)
{
    *ret = meal->customer;
}
void meal_set_customer(struct meal *meal, const struct customer *customer)
{
    meal->customer = (struct customer *)customer;
}

// returns number of times this meal has been picked up
int meal_picked_up(const struct meal *meal)
{
    return num_picked_up(meal->stats);
}
// returns number of times this meal's request has been completed
int meal_completed(const struct meal *meal)
{
    return num_completed(meal->stats);
}

// pick this meal up
void pick_meal_up(struct meal *meal)
{
    pick_up(meal->stats);
}
// drop off meal at customer
void drop_off_meal(struct meal *meal, struct customer *customer)
{
    complete(meal->stats);
    if (meal->customer == customer)
    {
        complete(customer->stats);
    }
    else
    {
        printf("meal customer mismatch in drop_off_meal\n");
    }
}

/**
 * Driver helpers
 */
enum LOCATION_TYPE
{
    CUSTOMER,
    MEAL,
    RESTAURANT
};

bool driver_location_correct(struct driver *driver, void *payload, enum LOCATION_TYPE type)
{
    struct location *location = NULL;
    struct restaurant *restaurant = NULL;
    switch(type)
    {
        case CUSTOMER:
            customer_get_location((struct customer *)payload, &location);
            break;
        case MEAL:
            meal_get_restaurant((struct meal *)payload, &restaurant);
            restaurant_get_location(restaurant, &location);
            break;
        case RESTAURANT:
            restaurant_get_location((struct restaurant *)payload, &location);
            break;
    }
    return location == driver->location;
}

void driver_pick_meal_up(struct driver *driver, struct meal *meal)
{
    driver->stats->num_meals_picked_up++;
    pick_meal_up(meal);
}
void driver_pick_customer_up(struct driver *driver, struct customer *customer)
{
    driver->stats->num_customers_picked_up++;
    pick_customer_up(customer);
}
void driver_drop_off_meal(struct driver *driver, struct meal *meal, struct customer *customer)
{
    driver->stats->num_meals_delivered++;
    drop_off_meal(meal, customer);
}
void driver_drop_off_customer(struct driver *driver, struct customer *customer, struct restaurant *restaurant)
{
    driver->stats->num_customers_delivered++;
    drop_off_customer(customer, restaurant);
}
void driver_drive_to_location(struct driver *driver, const struct location *location)
{
    driver->location = (struct location *)location;
    usleep(driver->params->sleepTime);
}

/**
 * Request helpers
 */
void swap_meals(struct meal *meal1, struct meal* meal2, bool preserveValidity)
{
    struct customer *customer1;
    struct customer *customer2;

    meal_get_customer(meal1, &customer1);
    meal_get_customer(meal2, &customer2);

    meal_set_customer(meal1, customer2);
    meal_set_customer(meal2, customer1);

    if (preserveValidity)
    {
        customer_set_meal(customer1, meal2);
        customer_set_meal(customer2, meal1);
    }
}

void init_restaurants(
    struct restaurant *restaurants, 
    const unsigned int num_restaurants,
    struct location *locations,
    const unsigned int num_locations)
{
    int randNum;

    for (unsigned int i = 0; i < num_restaurants; i++)
    {
        init_restaurant(&restaurants[i]);
        randNum = rand() % num_locations;
        restaurant_set_location(&restaurants[i], &locations[randNum]);
    }
}

void cleanup_restaurants(
    struct restaurant *restaurants, 
    const unsigned int num_restaurants)
{
    for (unsigned int i = 0; i < num_restaurants; i++)
    {
        cleanup_restaurant(&restaurants[i]);
    }
}

void init_requests(
    struct meal *meals,
    struct customer *customers,
    const unsigned int num_requests,
    struct restaurant *restaurants,
    const unsigned int num_restaurants,
    struct location *locations,
    const unsigned int num_locations)
{
    int randNum;

    for (unsigned int i = 0; i < num_requests; i++)
    {
        init_meal(&meals[i]);
        init_customer(&customers[i]);

        randNum = rand() % num_restaurants;

        meal_set_restaurant(&meals[i], &restaurants[randNum]);
        meal_set_customer(&meals[i], &customers[i]);
        customer_set_meal(&customers[i], &meals[i]);
        randNum = rand() % num_locations;
        customer_set_location(&customers[i], &locations[randNum]);
    }

    // shuffle customers to meals relationship, maintaining valid relationships
    for (int i = num_requests - 1; i >= 0; i--)
    {
        randNum = rand() % (i + 1);
        swap_meals(&meals[i], &meals[randNum], true);
    }
}

// we mix up a meal by switching the customers that two meals point to, but
//   not the meals that the customers point to
// so if we have meal A points to customer A, meal B points to customer B,
//   customer A points to meal A, and customer B points to meal B,
//   meal A will point to customer B, and meal B will point to customer A
//     after the mix up, but the customers will keep their canonical orderings
// we only mix up a meal if it hasn't been mixed up before
void mix_up_meals(
    struct meal *meals,
    const unsigned int num_requests)
{
    int randNum;

    for (unsigned int i = 0; i < num_requests; i++)
    {
        randNum = rand() % 2;

        // mix this meal up with the next meal from the same restaurant
        //   with 50/50 chance, but only if the meal hasn't been mixed up
        //   already
        if (randNum)
        {
            struct customer *purportedCustomer = NULL;
            meal_get_customer(&meals[i], &purportedCustomer);

            struct meal *purportedMeal = NULL;
            customer_get_meal(purportedCustomer, &purportedMeal);

            if (&meals[i] != purportedMeal)
            {
                // this meal has already been swapped
                continue;
            }

            struct restaurant *origRestaurant = NULL;
            meal_get_restaurant(&meals[i], &origRestaurant);

            for (unsigned int j = i + 1; j < num_requests; j++)
            {
                struct restaurant *curRestaurant = NULL;
                meal_get_restaurant(&meals[j], &curRestaurant);
                if (origRestaurant == curRestaurant)
                {
                    meal_get_customer(&meals[j], &purportedCustomer);
                    customer_get_meal(purportedCustomer, &purportedMeal);
                    if (&meals[j] != purportedMeal)
                    {
                        // this meal has already been swapped
                        continue;
                    }
                    swap_meals(&meals[i], &meals[j], false);
                    break;
                }
            }
        }
    }
}

void cleanup_requests(
    struct meal *meals,
    struct customer *customers,
    const unsigned int num_requests)
{
    for (unsigned int i = 0; i < num_requests; i++)
    {
        cleanup_meal(&meals[i]);
        cleanup_customer(&customers[i]);
    }
}

void dispatch_drivers(
    const struct location *driverHub,
    const struct ubler_params *ubler_params,
    struct driver_stats *driver_stats,
    struct driver *drivers,
    pthread_t *driver_threads,
    const unsigned int num_drivers)
{
    for (unsigned int i = 0; i < num_drivers; i++)
    {
        drivers[i].params = (struct ubler_params *)ubler_params;
        drivers[i].stats = &driver_stats[i];
        drivers[i].location = (struct location *)driverHub;
        driver_stats[i].num_meals_picked_up = 0;
        driver_stats[i].num_customers_picked_up = 0;
        driver_stats[i].num_meals_delivered = 0;
        driver_stats[i].num_customers_delivered = 0;

        if (pthread_create(&driver_threads[i], NULL, driver_thread, &drivers[i]) != 0)
        {
            //XXX(DF) should probably ahve some sort of error message here...
            exit(1);
        }
    }

    // wait for all drivers to finish
    for (unsigned int i = 0; i < num_drivers; i++)
    {
        pthread_join(driver_threads[i], NULL);
    }
}

void record_stats(
    const struct meal *meals,
    const struct customer *customers,
    const unsigned int num_requests,
    const struct driver_stats *driver_stats,
    const unsigned int num_drivers,
    struct ubler_stats *ret_stats)
{
    struct ubler_stats stats = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
    for (unsigned int i = 0; i < num_requests; i++)
    {
        stats.total_meals_picked_up += meal_picked_up(&meals[i]);
        stats.total_customers_picked_up += customer_picked_up(&customers[i]);

        if (meal_completed(&meals[i]) == 0)
        {
            stats.total_meals_ignored++;
        }
        if (customer_completed(&customers[i]) == 0)
        {
            stats.total_customers_ignored++;
        }

        if (meal_picked_up(&meals[i]) > 1 ||
            meal_completed(&meals[i]) > 1)
        {
            stats.total_meals_overserviced++;
        }
        if (customer_picked_up(&customers[i]) > 1 ||
            customer_completed(&customers[i]) > 1)
        {
            stats.total_customers_overserviced++;
        }
    }

    for (unsigned int i = 0; i < num_drivers; i++)
    {
        stats.driver_meals_picked_up += driver_stats[i].num_meals_picked_up;
        stats.driver_customers_picked_up += driver_stats[i].num_customers_picked_up;

        stats.driver_meals_delivered += driver_stats[i].num_meals_delivered;
        stats.driver_customers_delivered += driver_stats[i].num_customers_delivered;
    }

    *ret_stats = stats;
}

/**
 * Now, the actual request code
 */
#define MAX_REQUESTS 4096
#define MAX_DRIVERS 128
#define MAX_RESTAURANTS 32

#define NUM_LOCATIONS 32

struct meal *meals;
struct customer *customers;

pthread_mutex_t receive_request_lock;

unsigned int requests;

bool looped;
bool deliverMealsAndCustomers;

unsigned int mealPos;
unsigned int customerPos;

// puts either a customer or a meal in *ret
// if we put a customer in *ret, *isCustomer will be set to true
// otherwise, *isCustomer will be set to false
// 
// if ret==NULL or isCustomer==NULL, or we are out of
// meals or customers that need servicing, we will return -1
// 
// we return 0 on success
int receive_request(void **ret, bool *isCustomer)
{
    if (ret == NULL || isCustomer == NULL) {
        return -1;
    }
    pthread_mutex_lock(&receive_request_lock);
    *ret = NULL;
    looped = false;
    *isCustomer = (bool) (rand() % 2);
    if (!deliverMealsAndCustomers)
    {
        *isCustomer = true;
    }
    // purposefully reproducing code, because sometimes the code segfaults when
    //   variables are declared on the stack in this function
    if (*isCustomer)
    {
        if (customerPos == requests)
        {
            customerPos = 0;
        }
        for ( ; customerPos < requests; customerPos++)
        {
            if (!customer_completed(&customers[customerPos]))
            {
                *ret = (void *)&customers[customerPos];
                customerPos++;
                pthread_mutex_unlock(&receive_request_lock);
                return 0;
            }
            if (customerPos == requests - 1 && !looped)
            {
                looped = true;
                customerPos = 0;
            }
        }
    }
    if (!(*isCustomer))
    {
        if (mealPos == requests)
        {
            mealPos = 0;
        }
        for ( ; mealPos < requests; mealPos++)
        {
            if (!meal_completed(&meals[mealPos]))
            {
                *ret = (void *)&meals[mealPos];
                mealPos++;
                pthread_mutex_unlock(&receive_request_lock);
                return 0;
            }
            if (mealPos == requests - 1 && !looped)
            {
                looped = true;
                mealPos = 0;
            }
        }
    }
    pthread_mutex_unlock(&receive_request_lock);
    return -1;
}


// start num_requests with num_drivers and ubler_params
// put results of the simulation in stats
int create_and_run_requests(
    const unsigned int num_requests, 
    const unsigned int num_restaurants,
    const unsigned int num_drivers, 
    const struct ubler_params *ubler_params,
    struct ubler_stats *stats)
{
    if (num_requests > MAX_REQUESTS ||
        num_drivers > MAX_DRIVERS ||
        num_restaurants > MAX_RESTAURANTS)
    {
        return 1;
    }

    pthread_mutex_init(&receive_request_lock, NULL);
    requests = num_requests;
    deliverMealsAndCustomers = ubler_params->deliverMealsAndCustomers;

    meals = malloc(sizeof(struct meal) * requests);
    customers = malloc(sizeof(struct customer) * requests);
    struct restaurant *restaurants = malloc(sizeof(struct restaurant) * num_restaurants);
    struct location *locations = malloc(sizeof(struct location) * NUM_LOCATIONS);

    srand(time(NULL));

    // initialize restaurants
    init_restaurants(restaurants, num_restaurants, locations, NUM_LOCATIONS);

    // initialize meals, customers
    init_requests(meals, customers, num_requests, restaurants, num_restaurants, locations, NUM_LOCATIONS);

    // mix up customer's meals if necessary
    if (ubler_params->mixUpMeals)
    {
        mix_up_meals(meals, num_requests);
    }

    // dispatch actual drivers
    struct location driverHub;
    struct driver_stats *driver_stats = malloc(sizeof(struct driver_stats) * num_drivers);
    struct driver *drivers = malloc(sizeof(struct driver) * num_drivers);
    pthread_t *driver_threads = malloc(sizeof(pthread_t) * num_drivers);
    
    dispatch_drivers(
        &driverHub,
        ubler_params,
        driver_stats,
        drivers,
        driver_threads,
        num_drivers);

    // collect stats
    record_stats(meals, customers, num_requests, driver_stats, num_drivers, stats);

    // cleanup restaurants
    cleanup_restaurants(restaurants, num_restaurants);
    // cleanup meals, customers
    cleanup_requests(meals, customers, num_requests);

    pthread_mutex_destroy(&receive_request_lock);

    free(meals);
    free(customers);
    free(restaurants);
    free(locations);
    free(driver_stats);
    free(drivers);
    free(driver_threads);

    return 0;
}
