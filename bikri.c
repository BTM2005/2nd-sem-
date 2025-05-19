/*
POS SYSTEM FOR MART - FILE HANDLING BASED
Features:
- Admin and cashier login
- Add/update/delete/restock products (admin)
- Sell multiple products in a transaction (cart)
- Apply discount, print receipt
- Error handling for input
- Reports and sales logging
- Ability to view products
- Ability to view sales
- Ability to cancel process by pressing 0
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <time.h>

#define MAX_PRODUCTS 100
#define MAX_NAME 50
#define MAX_CART_ITEMS 20
#define USER_FILE "users.txt"
#define PRODUCT_FILE "products.txt"
#define SALES_FILE "sales.txt"
#define VOUCHER_FILE "vouchers.txt"
#define RECEIPT_FILE_PREFIX "receipt_"
#define MAX_VOUCHER_ID 100

typedef struct {
    int id;
    char name[MAX_NAME];
    float price;
    int stock;
} Product;

typedef struct {
    int id;
    char name[MAX_NAME];
    float price;
    int quantity;
} CartItem;

typedef struct {
    int voucher_id;
    float dueAmount;
} Voucher;

// Helper input functions
int getIntInput(const char *prompt, int min) {
    int val;
    while (1) {
        printf("%s (or 0 to cancel): ", prompt);
        if (scanf("%d", &val) != 1) {
            printf("Invalid input. Try again.\n");
            while (getchar() != '\n');
        } else if (val == 0) {
            return 0;
        } else if (val < min) {
            printf("Input must be >= %d\n", min);
        } else return val;
    }
}

float getFloatInput(const char *prompt, float min) {
    float val;
    while (1) {
        printf("%s (or 0 to cancel): ", prompt);
        if (scanf("%f", &val) != 1) {
            printf("Invalid input. Try again.\n");
            while (getchar() != '\n');
        } else if (val == 0) {
            return 0;
        } else if (val < min) {
            printf("Input must be >= %.2f\n", min);
        } else return val;
    }
}

void getStringInput(const char *prompt, char *buffer, int len) {
    printf("%s", prompt);
    if (fgets(buffer, len, stdin) != NULL) {
        // Remove trailing newline character if present
        size_t input_len = strlen(buffer);
        if (input_len > 0 && buffer[input_len - 1] == '\n') {
            buffer[input_len - 1] = '\0';
        }
    } else {
        // Handle potential input error (e.g., Ctrl+D)
        buffer[0] = '\0'; // Ensure the buffer is empty
        printf("Input error or end-of-file.\n");
    }
}

int generateVoucherID() {
    static int nextVoucherId = 1;
    return nextVoucherId++;
}

int productExists(int id) {
    FILE *fp = fopen(PRODUCT_FILE, "r");
    if (!fp) return 0;
    int pid;
    char name[MAX_NAME];
    float price;
    int stock;
    while (fscanf(fp, "%d %s %f %d", &pid, name, &price, &stock) != EOF) {
        if (pid == id) {
            fclose(fp);
            return 1;
        }
    }
    fclose(fp);
    return 0;
}

int loadProducts(Product products[]) {
    FILE *fp = fopen(PRODUCT_FILE, "r");
    if (!fp) return 0;
    int count = 0;
    while (fscanf(fp, "%d %s %f %d", &products[count].id, products[count].name,
                  &products[count].price, &products[count].stock) != EOF) {
        count++;
    }
    fclose(fp);
    return count;
}

void saveProducts(Product products[], int count) {
    FILE *fp = fopen(PRODUCT_FILE, "w");
    for (int i = 0; i < count; i++) {
        fprintf(fp, "%d %s %.2f %d\n", products[i].id, products[i].name,
                products[i].price, products[i].stock);
    }
    fclose(fp);
}

void addProduct() {
    Product p;
    printf("\n--- Add New Product ---\n");
    while (1) {
        p.id = getIntInput("Enter product ID", 1);
        if (p.id == 0) break;
        if (productExists(p.id)) printf("ID exists. Try again.\n");
        else {
            getStringInput("Enter product name: ", p.name, MAX_NAME);
            p.price = getFloatInput("Enter price", 0);
            if (p.price == 0) break;
            p.stock = getIntInput("Enter stock", 0);
            if (p.stock == 0) break;

            FILE *fp = fopen(PRODUCT_FILE, "a");
            if (!fp) {
                perror("Error opening file");
                return;
            }
            fprintf(fp, "%d %s %.2f %d\n", p.id, p.name, p.price, p.stock);
            fclose(fp);
            printf("Product added!\n");
        }
    }
}

void restockProduct(Product products[], int count) {
    while (1) {
        int id = getIntInput("Enter Product ID to restock", 1);
        if (id == 0) break;
        int found = 0;
        for (int i = 0; i < count; i++) {
            if (products[i].id == id) {
                int qty = getIntInput("Enter quantity to add", 1);
                if (qty == 0) break;
                products[i].stock += qty;
                saveProducts(products, count);
                printf("Stock updated.\n");
                found = 1;
                break;
            }
        }
        if (!found) printf("Product not found.\n");
    }
}

void deleteProduct(Product products[], int *count) {
    while (1) {
        int id = getIntInput("Enter Product ID to delete", 1);
        if (id == 0) break;
        int index = -1;
        for (int i = 0; i < *count; i++) {
            if (products[i].id == id) {
                index = i;
                break;
            }
        }
        if (index >= 0 && index < *count) { // Add boundary check
            for (int i = index; i < *count - 1; i++) {
                products[i] = products[i + 1];
            }
            (*count)--;
            saveProducts(products, *count);
            printf("Product deleted.\n");
        } else printf("Product not found.\n");
    }
}
void recordDueAmount(int voucherId, float dueAmount) {
    FILE *fp = fopen(VOUCHER_FILE, "a");
    if (fp) {
        fprintf(fp, "%d %.2f\n", voucherId, dueAmount);
        fclose(fp);
        printf("Due amount recorded with Voucher ID: %d\n", voucherId);
    } else {
        perror("Error opening voucher file for writing");
    }
}

bool findVoucher(int voucherId, Voucher *voucher) {
    FILE *fp = fopen(VOUCHER_FILE, "r");
    if (!fp) {
        perror("Error opening voucher file for reading");
        return false;
    }
    Voucher tempVoucher;
    while (fscanf(fp, "%d %f", &tempVoucher.voucher_id, &tempVoucher.dueAmount) == 2) {
        if (tempVoucher.voucher_id == voucherId) {
            *voucher = tempVoucher;
            fclose(fp);
            return true;
        }
    }
    fclose(fp);
    return false;
}

void updateVoucherFile(int clearedVoucherId) {
    FILE *fp = fopen(VOUCHER_FILE, "r");
    if (!fp) {
        perror("Error opening voucher file for reading");
        return;
    }
    FILE *tempFp = fopen("temp_vouchers.txt", "w");
    if (!tempFp) {
        perror("Error opening temporary voucher file for writing");
        fclose(fp);
        return;
    }

    Voucher tempVoucher;
    while (fscanf(fp, "%d %f", &tempVoucher.voucher_id, &tempVoucher.dueAmount) == 2) {
        if (tempVoucher.voucher_id != clearedVoucherId) {
            fprintf(tempFp, "%d %.2f\n", tempVoucher.voucher_id, tempVoucher.dueAmount);
        }
    }
    fclose(fp);
    fclose(tempFp);
    remove(VOUCHER_FILE);
    rename("temp_vouchers.txt", VOUCHER_FILE);
    printf("Voucher ID %d cleared successfully.\n", clearedVoucherId);
}

void clearDue() {
    int voucherId = getIntInput("Enter Voucher ID to clear due", 1);
    if (voucherId == 0) return;

    Voucher voucher;
    if (findVoucher(voucherId, &voucher)) {
        printf("Voucher ID: %d, Due Amount: %.2f\n", voucher.voucher_id, voucher.dueAmount);
        float amountPaid = getFloatInput("Enter amount paid", 0);
        if (amountPaid == 0) return;

        if (amountPaid >= voucher.dueAmount) {
            float change = amountPaid - voucher.dueAmount;
            printf("Amount cleared. Change: %.2f\n", change);
            updateVoucherFile(voucherId);
            // Optionally record this transaction in the sales log
            FILE *fp = fopen(SALES_FILE, "a");
            if (fp) {
                fprintf(fp, "Cleared Due - Voucher ID: %d, Due: %.2f, Paid: %.2f, Change: %.2f\n",
                        voucherId, voucher.dueAmount, amountPaid, change);
                fclose(fp);
            } else {
                perror("Error opening sales file for writing");
            }
        } else {
            printf("Amount paid is less than the due amount.\n");
        }
    } else {
        printf("Voucher ID %d not found.\n", voucherId);
    }
}

char* generateReceiptFilename() {
    time_t timer;
    struct tm* tm_info;
    char* filename = (char*)malloc(sizeof(char) * 100);
    if (filename == NULL) {
        perror("Memory allocation failed for receipt filename");
        return NULL;
    }

    time(&timer);
    tm_info = localtime(&timer);

    strftime(filename, 100, RECEIPT_FILE_PREFIX "%Y%m%d_%H%M%S.txt", tm_info);
    return filename;
}

void printReceipt(CartItem cart[], int cartSize, float total, float discount, float discountedTotal, float cashPaid, float change, int voucherId, float dueAmount) {
    char* filename = generateReceiptFilename();
    if (filename == NULL) return;

    FILE* receiptFile = fopen(filename, "w");
    if (receiptFile == NULL) {
        perror("Error opening receipt file for writing");
        free(filename);
        return;
    }

    fprintf(receiptFile, "\n--- Mart POS System - Receipt ---\n");
    time_t timer;
    time(&timer);
    fprintf(receiptFile, "Date: %s", ctime(&timer));
    fprintf(receiptFile, "-----------------------------------\n");
    fprintf(receiptFile, "%-20s %-5s %-10s\n", "Item", "Qty", "Price");
    fprintf(receiptFile, "-----------------------------------\n");
    for (int i = 0; i < cartSize; i++) {
        fprintf(receiptFile, "%-20s %-5d %.2f\n", cart[i].name, cart[i].quantity, cart[i].quantity * cart[i].price);
    }
    fprintf(receiptFile, "-----------------------------------\n");
    fprintf(receiptFile, "Subtotal:             %.2f\n", total);
    fprintf(receiptFile, "Discount (%.2f%%):    -%.2f\n", discount, (total * discount / 100));
    fprintf(receiptFile, "Total:                %.2f\n", discountedTotal);
    fprintf(receiptFile, "-----------------------------------\n");
    if (cashPaid >= discountedTotal) {
        fprintf(receiptFile, "Cash Paid:            %.2f\n", cashPaid);
        fprintf(receiptFile, "Change:               %.2f\n", change);
    } else {
        fprintf(receiptFile, "Cash Paid:            %.2f\n", cashPaid);
        fprintf(receiptFile, "Due Amount:           %.2f\n", dueAmount);
        fprintf(receiptFile, "Voucher ID:           %d\n", voucherId);
    }
    fprintf(receiptFile, "-----------------------------------\n");
    fprintf(receiptFile, "Thank you for shopping with us!\n");

    fclose(receiptFile);
    printf("Receipt printed to file: %s\n", filename);
    free(filename);
}

void checkout(Product products[], int *count) {
    CartItem cart[MAX_CART_ITEMS];
    int cartSize = 0;
    float total = 0;

    printf("\n--- Checkout ---\n");
    printf("Enter product ID to add to cart (0 to finish):\n");

    while (cartSize < MAX_CART_ITEMS) {
        int id = getIntInput("Enter product ID", 0);
        if (id == 0) {
            break;
        }

        int productIndex = -1;
        for (int i = 0; i < *count; i++) {
            if (products[i].id == id) {
                productIndex = i;
                break;
            }
        }

        if (productIndex == -1) {
            printf("Product with ID %d not found.\n", id);
            continue;
        }

        printf("Product: %s (Stock: %d)\n", products[productIndex].name, products[productIndex].stock);
        int qty = getIntInput("Enter quantity", 1);

        if (qty == 0) {
            continue;
        }

        if (products[productIndex].stock < qty) {
            printf("Not enough stock for %s. Available: %d\n", products[productIndex].name, products[productIndex].stock);
        } else {
            products[productIndex].stock -= qty;

            int alreadyInCart = -1;
            for (int i = 0; i < cartSize; i++) {
                if (cart[i].id == products[productIndex].id) {
                    alreadyInCart = i;
                    break;
                }
            }

            if (alreadyInCart != -1) {
                cart[alreadyInCart].quantity += qty;
            } else {
                cart[cartSize].id = products[productIndex].id;
                strcpy(cart[cartSize].name, products[productIndex].name);
                cart[cartSize].price = products[productIndex].price;
                cart[cartSize].quantity = qty;
                cartSize++;
            }
            total += qty * products[productIndex].price;
            printf("%d x %s added to cart. Current total: %.2f\n", qty, products[productIndex].name, total);
        }
    }

    if (cartSize == 0) {
        printf("\nNo items purchased.\n");
        return;
    }

    printf("\n--- Cart Items ---\n");
    for (int i = 0; i < cartSize; i++) {
        printf("%s x%d = %.2f\n", cart[i].name, cart[i].quantity,
               cart[i].quantity * cart[i].price);
    }
    printf("Subtotal: %.2f\n", total);

    float discount = getFloatInput("Enter discount (in %)", 0);
    float discountedTotal = total - (total * discount / 100);

    printf("Discount (%.2f%%): -%.2f\n", discount, (total * discount / 100));
    printf("Total: %.2f\n", discountedTotal);

    float cashPaid = getFloatInput("Enter cash paid", 0);
    float change = 0;
    int voucherId = 0;
    float dueAmount = 0;

    if (cashPaid >= discountedTotal) {
        change = cashPaid - discountedTotal;
        printf("Change: %.2f\n", change);
        FILE *fp = fopen(SALES_FILE, "a");
        if (fp) {
            fprintf(fp, "Total: %.2f Discount: %.2f Final: %.2f Cash Paid: %.2f Change: %.2f\n",
                    total, discount, discountedTotal, cashPaid, change);
            fclose(fp);
            printf("Sale recorded.\n");
        } else {
            perror("Error opening sales file for writing");
        }
        printReceipt(cart, cartSize, total, discount, discountedTotal, cashPaid, change, voucherId, dueAmount);
    } else {
        dueAmount = discountedTotal - cashPaid;
        printf("Due amount: %.2f\n", dueAmount);
        voucherId = generateVoucherID();
        recordDueAmount(voucherId, dueAmount);
        FILE *fp = fopen(SALES_FILE, "a");
        if (fp) {
            fprintf(fp, "Total: %.2f Discount: %.2f Final: %.2f Cash Paid: %.2f Due: %.2f (Voucher ID: %d)\n",
                    total, discount, discountedTotal, cashPaid, dueAmount, voucherId);
            fclose(fp);
            printf("Sale recorded with due amount.\n");
        } else {
            perror("Error opening sales file for writing");
        }
        printReceipt(cart, cartSize, total, discount, discountedTotal, cashPaid, change, voucherId, dueAmount);
    }

    saveProducts(products, *count);
    printf("Product stock updated.\n");
}
void adminMenu() {
    Product products[MAX_PRODUCTS];
    int count = loadProducts(products);
    int choice;
    do {
        printf("\n--- Admin Menu ---\n");
        printf("1. Add Product\n2. Restock Product\n3. Delete Product\n4. View Products\n5. Clear Customer Due\n6. Logout\n");
        choice = getIntInput("Enter choice: ", 1);
        switch (choice) {
            case 1: addProduct(); break;
            case 2: restockProduct(products, count); break;
            case 3: deleteProduct(products, &count); break;
            case 4:
            printf("\n--- Product List ---\n");
            printf("%-5s %-20s %-10s %-8s\n", "ID", "Name", "Price", "Stock");
            printf("---------------------------------------------------\n");
            for (int i = 0; i < count; i++) {
                printf("%-5d %-20s %-10.2f %-8d\n", products[i].id, products[i].name,
                       products[i].price, products[i].stock);
            }
            printf("---------------------------------------------------\n");
            break;
            case 5: clearDue(); break;
        }
    } while (choice != 6);
}

void cashierMenu() {
    Product products[MAX_PRODUCTS];
    int count = loadProducts(products);
    int choice;
    do {
        printf("\n--- Cashier Menu ---\n");
        printf("1. Checkout\n2. View Products\n3. Clear Customer Due\n4. Logout\n");
        choice = getIntInput("Enter choice: ", 1);
        switch (choice) {
            case 1: checkout(products, &count); break;
            case 2:
            printf("\n--- Product List ---\n");
            printf("%-5s %-20s %-10s %-8s\n", "ID", "Name", "Price", "Stock");
            printf("---------------------------------------------------\n");
            for (int i = 0; i < count; i++) {
                printf("%-5d %-20s %-10.2f %-8d\n", products[i].id, products[i].name,
                       products[i].price, products[i].stock);
            }
            printf("---------------------------------------------------\n");
            break;
            case 3: clearDue(); break;
        }
    } while (choice != 4);
}

void login() {
    char username[30];
    char password[30];
    char role[10];

    printf("\n--- Login ---\n");

    getStringInput("Username: ", username, sizeof(username));
    getStringInput("Password: ", password, sizeof(password));

    FILE *fp = fopen(USER_FILE, "r");
    if (!fp) {
        perror("Error opening user file");
        printf("The program will now exit.\n");
        exit(EXIT_FAILURE);
        return;
    }

    char u[30];
    char p[30];
    char r[10];
    int found = 0;

    while (fscanf(fp, "%29s %29s %9s", u, p, r) == 3) {
        if (strcmp(username, u) == 0 && strcmp(password, p) == 0) {
            found = 1;
            strcpy(role, r);
            break;
        }
    }
    fclose(fp);

    if (!found) {
        printf("Invalid credentials.\n");
    } else if (strcmp(role, "admin") == 0) {
        adminMenu();
    } else if (strcmp(role, "cashier") == 0) {
        cashierMenu();
    } else {
        printf("Unknown role: %s. Please contact system administrator.\n", role);
    }
}

int main() {
    printf("\nWelcome to Mart POS System\n");
    while (1) {
        login();
        char again;
        printf("\nLogout successful. Login again? (y/n): ");
        if (scanf(" %c", &again) != 1) {
            printf("Invalid input. Exiting.\n");
            break;
        }
        if (again != 'y' && again != 'Y') {
            break;
        }
        int c;
        while ((c = getchar()) != '\n' && c != EOF);
    }
    printf("Exiting POS System.\n");
    return 0;
}