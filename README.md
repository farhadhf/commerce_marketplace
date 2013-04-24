Commerce Marketplace
====================

Overview
--------
This projects is a set of modules which lets you create a marketplace using
Drupal Commerce. Please note that it's not ready yet and you probably won't be
able to get it working just by getting the code and installing the module.

Features
--------
* Stores: The module provides a new entity type for stores. Each user can have
  multiple stores and each store can have multiple members. Each product has an
  Store reference which determines which store owns that product. An
  entityreference Selection plugin is developed to restrict the store
  entityreference autocomplete results to the stores that the user is a member
  of.This feature is completed and working.
* Store access control: There are 3 global store roles (non-member, member and
  store administrator). You can also create store-specific-roles yourself and
  assign permissions to each role (just like Drupal core permissions and
  roles). This feature is completed working.
* Marketplace orders: A new order type which is used as the top-level orders and
  is used to handle customer carts. Customers only see this type of orders.
* Store orders: A new order type which is used handle orders for each store.
  Each Store order has a reference to the original Marketplace order. Every time
  a product is added to a marketplace order, based on its Store reference, If
  there are no other products from the same store in the Marketplace order a new
  Store order gets created and the product also gets added to the new Store
  order, otherwise the product is added to an existing Store order which contains
  products from the same store. Each Store order is updated whenever something
  changes in the associated Marketplace order - All orders are synced at all
  times. Also, store members with sufficient permission, can edit the Store
  orders. This feature is completed and working.
* Shipping fees: A new shipping method is developed which calculates shipping
  rates for each store and adds the sum of calculated shipping fees to the
  marketplace order. Currently, because of commerce_shipping's limitations,
  there are problems with calculating the shipping rates based on the store
  preferences using shipping methods like UPS. But the flat rate shipping method
  works just fine.
* Payment to stores: I was thinking of using commerce_funds for handling the
  payments. But commerce_funds has some limitations, It uses an specific order
  and product type for crediting user accounts and it is hardcoded to work with
  user entities - it doesn't work with any other entity type (e.g. stores).
  I can either for the commerce_funds module and make some adjustments so that
  it works in the marketplace system or we'll have to come up with another
  solution for this problem.

