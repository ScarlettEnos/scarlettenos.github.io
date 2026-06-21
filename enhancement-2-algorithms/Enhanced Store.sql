-- I added categories and reorder levels so I could track inventory in more detail.

DROP TABLE IF EXISTS clothingstore;

CREATE TABLE clothingstore (
    id INTEGER PRIMARY KEY,
    type TEXT NOT NULL,
    category TEXT NOT NULL,
    price REAL NOT NULL,
    fabric TEXT,
    quantity INTEGER NOT NULL,
    reorder_level INTEGER NOT NULL
);

-- I added more products to make the inventory more realistic.
INSERT INTO clothingstore VALUES
(1, 'jeans', 'bottoms', 15.00, 'denim', 20, 8),
(2, 'blouses', 'tops', 10.00, 'cotton', 25, 10),
(3, 'purses', 'accessories', 20.00, 'leather', 10, 5),
(4, 'shoes', 'footwear', 20.00, 'miscellaneous', 10, 6),
(5, 'skirts', 'bottoms', 18.00, 'cotton', 7, 8),
(6, 'jackets', 'outerwear', 35.00, 'denim', 4, 5),
(7, 'scarves', 'accessories', 8.00, 'polyester', 15, 7),
(8, 'sandals', 'footwear', 22.00, 'leather', 3, 5);

-- I displayed all products currently in the inventory.
SELECT * FROM clothingstore;

-- I sorted the products from highest price to lowest price.
SELECT * FROM clothingstore
ORDER BY price DESC;

-- I displayed products that cost less than $20.
SELECT * FROM clothingstore
WHERE price < 20
ORDER BY price ASC;

-- I calculated the total number of items currently in inventory.
SELECT SUM(quantity) AS total_inventory_quantity
FROM clothingstore;

-- I added a way to see which products are worth the most based on price and quantity.
SELECT
    type,
    category,
    price,
    quantity,
    price * quantity AS inventory_value
FROM clothingstore
ORDER BY inventory_value DESC;

-- I calculated the total value of everything currently in inventory.
SELECT SUM(price * quantity) AS total_inventory_value
FROM clothingstore;

-- I added a query to identify products that are starting to run low.
SELECT
    type,
    category,
    quantity,
    reorder_level
FROM clothingstore
WHERE quantity <= reorder_level
ORDER BY quantity ASC;

-- I grouped products by category to make the inventory easier to review.
SELECT
    category,
    COUNT(*) AS number_of_products,
    SUM(quantity) AS total_quantity,
    SUM(price * quantity) AS category_inventory_value
FROM clothingstore
GROUP BY category
ORDER BY category_inventory_value DESC;

-- I grouped products by fabric to compare the different materials in inventory.
SELECT
    fabric,
    COUNT(*) AS number_of_products,
    SUM(quantity) AS total_quantity
FROM clothingstore
GROUP BY fabric
ORDER BY total_quantity DESC;

-- I added a query to show which products should be reordered first.
SELECT
    type,
    category,
    quantity,
    reorder_level,
    reorder_level - quantity AS units_below_reorder_level
FROM clothingstore
WHERE quantity < reorder_level
ORDER BY units_below_reorder_level DESC;