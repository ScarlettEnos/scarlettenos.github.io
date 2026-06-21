-- I enhanced my original inventory database by splitting the single table into related tables.
-- This makes the database more organized and easier to expand.
DROP VIEW IF EXISTS LowStockProducts;
DROP VIEW IF EXISTS InventoryValueReport;
DROP VIEW IF EXISTS SupplierInventoryReport;
DROP VIEW IF EXISTS CategoryInventoryReport;

DROP TABLE IF EXISTS inventory;
DROP TABLE IF EXISTS products;
DROP TABLE IF EXISTS suppliers;
DROP TABLE IF EXISTS categories;

-- I created a separate table for product categories.
CREATE TABLE categories (
    category_id INTEGER PRIMARY KEY,
    category_name TEXT NOT NULL
);

-- I created a separate table for suppliers.
CREATE TABLE suppliers (
    supplier_id INTEGER PRIMARY KEY,
    supplier_name TEXT NOT NULL,
    contact_email TEXT
);

-- I created a products table that connects each item to a category and supplier.
CREATE TABLE products (
    product_id INTEGER PRIMARY KEY,
    product_name TEXT NOT NULL,
    category_id INTEGER NOT NULL,
    supplier_id INTEGER NOT NULL,
    price REAL NOT NULL,
    fabric TEXT,
    FOREIGN KEY (category_id) REFERENCES categories(category_id),
    FOREIGN KEY (supplier_id) REFERENCES suppliers(supplier_id)
);

-- I created an inventory table to track quantity and reorder levels separately.
CREATE TABLE inventory (
    inventory_id INTEGER PRIMARY KEY,
    product_id INTEGER NOT NULL,
    quantity INTEGER NOT NULL,
    reorder_level INTEGER NOT NULL,
    FOREIGN KEY (product_id) REFERENCES products(product_id)
);

-- I added the product categories.
INSERT INTO categories VALUES
(1, 'bottoms'),
(2, 'tops'),
(3, 'accessories'),
(4, 'footwear'),
(5, 'outerwear');

-- I added supplier information to make the database more realistic.
INSERT INTO suppliers VALUES
(1, 'Scarlett''s Boutique', 'boutique@scarletts.com'),
(2, 'Scarlett''s Apparel', 'apparel@scarletts.com'),
(3, 'Scarlett''s Accessories', 'accessories@scarletts.com');

-- I added product information and connected each product to a category and supplier.
INSERT INTO products VALUES
(1, 'jeans', 1, 1, 15.00, 'denim'),
(2, 'blouses', 2, 2, 10.00, 'cotton'),
(3, 'purses', 3, 3, 20.00, 'leather'),
(4, 'shoes', 4, 3, 20.00, 'miscellaneous'),
(5, 'skirts', 1, 2, 18.00, 'cotton'),
(6, 'jackets', 5, 1, 35.00, 'denim'),
(7, 'scarves', 3, 3, 8.00, 'polyester'),
(8, 'sandals', 4, 3, 22.00, 'leather');

-- I added inventory information separately from the product details.
INSERT INTO inventory VALUES
(1, 1, 20, 8),
(2, 2, 25, 10),
(3, 3, 10, 5),
(4, 4, 10, 6),
(5, 5, 7, 8),
(6, 6, 4, 5),
(7, 7, 15, 7),
(8, 8, 3, 5);

-- I used joins to bring product, category, supplier, and inventory details together.
SELECT
    products.product_name,
    categories.category_name,
    suppliers.supplier_name,
    products.price,
    products.fabric,
    inventory.quantity,
    inventory.reorder_level
FROM products
JOIN categories ON products.category_id = categories.category_id
JOIN suppliers ON products.supplier_id = suppliers.supplier_id
JOIN inventory ON products.product_id = inventory.product_id;

-- I created a view so I can quickly see which products are running low.
CREATE VIEW LowStockProducts AS
SELECT
    products.product_name,
    categories.category_name,
    suppliers.supplier_name,
    inventory.quantity,
    inventory.reorder_level
FROM products
JOIN categories ON products.category_id = categories.category_id
JOIN suppliers ON products.supplier_id = suppliers.supplier_id
JOIN inventory ON products.product_id = inventory.product_id
WHERE inventory.quantity <= inventory.reorder_level;

SELECT * FROM LowStockProducts;

-- I created a view to show the total inventory value for each product.
CREATE VIEW InventoryValueReport AS
SELECT
    products.product_name,
    categories.category_name,
    suppliers.supplier_name,
    products.price,
    inventory.quantity,
    products.price * inventory.quantity AS inventory_value
FROM products
JOIN categories ON products.category_id = categories.category_id
JOIN suppliers ON products.supplier_id = suppliers.supplier_id
JOIN inventory ON products.product_id = inventory.product_id;

SELECT * FROM InventoryValueReport;

-- I created a view to summarize inventory value by supplier.
CREATE VIEW SupplierInventoryReport AS
SELECT
    suppliers.supplier_name,
    COUNT(products.product_id) AS number_of_products,
    SUM(products.price * inventory.quantity) AS supplier_inventory_value
FROM suppliers
JOIN products ON suppliers.supplier_id = products.supplier_id
JOIN inventory ON products.product_id = inventory.product_id
GROUP BY suppliers.supplier_name;

SELECT * FROM SupplierInventoryReport;

-- I created a view to summarize inventory by category.
CREATE VIEW CategoryInventoryReport AS
SELECT
    categories.category_name,
    COUNT(products.product_id) AS number_of_products,
    SUM(inventory.quantity) AS total_quantity,
    SUM(products.price * inventory.quantity) AS category_inventory_value
FROM categories
JOIN products ON categories.category_id = products.category_id
JOIN inventory ON products.product_id = inventory.product_id
GROUP BY categories.category_name;

SELECT * FROM CategoryInventoryReport;