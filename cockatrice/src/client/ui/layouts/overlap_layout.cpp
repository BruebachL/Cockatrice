#include "overlap_layout.h"

#include <QDebug>

/**
 * @class OverlapLayout
 * @brief Custom layout class to arrange widgets with overlapping positions.
 *
 * The OverlapLayout class is a QLayout subclass that arranges child widgets
 * in an overlapping configuration, allowing control over the overlap percentage
 * and the number of rows or columns based on the chosen layout direction. This
 * layout is particularly useful for visualizing elements that need to partially
 * stack over one another, either horizontally or vertically.
 */

/**
 * @brief Constructs an OverlapLayout with the specified parameters.
 *
 * Initializes a new OverlapLayout with the given overlap percentage, row or column limit,
 * and layout direction. The overlap percentage determines how much each widget will
 * overlap with the previous one. If maxColumns or maxRows are set to zero, it implies
 * no limit in that respective dimension.
 *
 * @param overlapPercentage An integer representing the percentage of overlap between items (0-100).
 * @param maxColumns The maximum number of columns allowed in the layout when in horizontal orientation (0 for
 * unlimited).
 * @param maxRows The maximum number of rows allowed in the layout when in vertical orientation (0 for unlimited).
 * @param direction The orientation direction of the layout, either Qt::Horizontal or Qt::Vertical.
 * @param parent The parent widget of this layout.
 */
OverlapLayout::OverlapLayout(QWidget *parent,
                             int overlapPercentage,
                             int maxColumns,
                             int maxRows,
                             Qt::Orientation direction)
    : QLayout(parent), overlapPercentage(overlapPercentage), maxColumns(maxColumns), maxRows(maxRows),
      direction(direction)
{
}

/**
 * @brief Destructor for OverlapLayout, ensuring cleanup of all layout items.
 *
 * Iterates through all layout items and deletes them. This prevents memory
 * leaks by removing all child QLayoutItems stored in the layout.
 */
OverlapLayout::~OverlapLayout()
{
    QLayoutItem *item;
    while ((item = OverlapLayout::takeAt(0))) {
        delete item;
    }
}

/**
 * @brief Adds a new item to the layout.
 *
 * Appends a QLayoutItem to the internal list, allowing it to be positioned within the
 * layout during the next geometry update. This method does not directly arrange the
 * items; it merely adds them to the layout’s tracking.
 *
 * @param item Pointer to the QLayoutItem being added to this layout.
 */
void OverlapLayout::addItem(QLayoutItem *item)
{
    if (item != nullptr) {
        itemList.append(item);
    }
}

/**
 * @brief Retrieves the total count of items within the layout.
 *
 * Returns the number of items stored in the layout's internal item list.
 * This count reflects how many widgets or spacers the layout is currently managing.
 *
 * @return Integer count of items in the layout.
 */
int OverlapLayout::count() const
{
    return itemList.size();
}

/**
 * @brief Provides access to a layout item at a specified index.
 *
 * Allows retrieval of a QLayoutItem from the layout’s internal list
 * by index. If the index is out of bounds, this function will return nullptr.
 *
 * @param index The index of the desired item.
 * @return Pointer to the QLayoutItem at the specified index, or nullptr if index is invalid.
 */
QLayoutItem *OverlapLayout::itemAt(int index) const
{
    return itemList.value(index);
}

/**
 * @brief Removes and returns a layout item at the specified index.
 *
 * Removes a QLayoutItem from the layout at the given index, reducing the layout's count.
 * If the index is invalid, this function returns nullptr without any effect.
 *
 * @param index The index of the item to remove.
 * @return Pointer to the removed QLayoutItem, or nullptr if index is invalid.
 */
QLayoutItem *OverlapLayout::takeAt(int index)
{
    return itemList.takeAt(index);
}

/**
 * @brief Sets the geometry for the layout items, arranging them with the specified overlap.
 * @param rect The rectangle defining the area within which the layout should arrange items.
 */
void OverlapLayout::setGeometry(const QRect &rect)
{
    // Call the base class implementation to ensure standard layout behavior.
    QLayout::setGeometry(rect);

    // If there are no items to layout, exit early.
    if (itemList.isEmpty()) {
        return;
    }

    // Get the parent widget for size and margin calculations.
    QWidget *parentWidget = this->parentWidget();
    if (!parentWidget) {
        return;
    }

    // Calculate available width and height, subtracting the parent's margins.
    int availableWidth = parentWidget->width();
    int availableHeight = parentWidget->height();
    QMargins margins = parentWidget->contentsMargins();
    availableWidth -= margins.left() + margins.right();
    availableHeight -= margins.top() + margins.bottom();

    // Determine the maximum item width and height among all layout items.
    int maxItemWidth = 0;
    int maxItemHeight = 0;
    for (const QLayoutItem *item : itemList) {
        if (item->widget()) {
            QSize itemSize = item->widget()->sizeHint();
            maxItemWidth = std::max(maxItemWidth, itemSize.width());
            maxItemHeight = std::max(maxItemHeight, itemSize.height());
        }
    }

    // Calculate the overlap offsets based on the layout direction and overlap percentage.
    int overlapOffsetWidth = (direction == Qt::Horizontal) ? (maxItemWidth * overlapPercentage / 100) : 0;
    int overlapOffsetHeight = (direction == Qt::Vertical) ? (maxItemHeight * overlapPercentage / 100) : 0;

    // Determine the number of columns and rows based on layout constraints and available space.

    int columns =
        (direction == Qt::Horizontal) // If the layout is horizontal,
            ? (maxColumns > 0         // Check if a maximum column count is defined.
                   ? std::min(maxColumns,
                              (availableWidth + overlapOffsetWidth) /
                                  (maxItemWidth -
                                   overlapOffsetWidth)) // If defined,
                                                        // Calculate the number of columns that can fit in the available
                                                        // width, considering the item size and overlap. Use `std::min`
                                                        // to ensure we do not exceed the maximum allowed columns.
                   : INT_MAX) // If maxColumns is not defined (0 or negative), allow as many columns as possible.
            : INT_MAX; // If the layout is not horizontal (likely vertical), set columns to an arbitrarily large number
                       // (INT_MAX),
    // because column count is not relevant in vertical layouts.

    int rows =
        (direction == Qt::Vertical) // If the layout is vertical,
            ? (maxRows > 0          // Check if a maximum row count is defined.
                   ? std::min(
                         maxRows,
                         (availableHeight + overlapOffsetHeight) /
                             (maxItemHeight -
                              overlapOffsetHeight)) // If defined,
                                                    // Calculate the number of rows that can fit in the available
                                                    // height, considering the item size and overlap. Use `std::min` to
                                                    // ensure we do not exceed the maximum allowed rows.
                   : INT_MAX) // If maxRows is not defined (0 or negative), allow as many rows as possible.
            : INT_MAX; // If the layout is not vertical (likely horizontal), set rows to an arbitrarily large number
                       // (INT_MAX), because row count is not relevant in horizontal layouts.

    // Initialize row and column indices.
    int currentRow = 0;
    int currentColumn = 0;

    // Loop through all items and position them based on the calculated offsets.
    for (int i = 0; i < itemList.size(); ++i) {
        QLayoutItem *item = itemList.at(i);
        if (item != nullptr) {
            // Calculate the position of the current item.
            int xPos = rect.left() + currentColumn * (maxItemWidth - overlapOffsetWidth);
            int yPos = rect.top() + currentRow * (maxItemHeight - overlapOffsetHeight);
            item->setGeometry(QRect(xPos, yPos, maxItemWidth, maxItemHeight));

            // Update row and column indices based on the layout direction.
            if (direction == Qt::Horizontal) {
                currentColumn++;
                if (currentColumn >= columns) {
                    currentColumn = 0;
                    currentRow++;
                }
            } else {
                currentRow++;
                if (currentRow >= rows) {
                    currentRow = 0;
                    currentColumn++;
                }
            }
        }
    }
}

/**
 * @brief Calculates the preferred size for the layout, considering overlap and orientation.
 * @return The preferred layout size as a QSize object.
 */
QSize OverlapLayout::calculatePreferredSize() const
{
    // Initialize the preferred size to zero.
    QSize preferredSize(0, 0);

    // Determine the maximum item dimensions.
    int maxItemWidth = 0;
    int maxItemHeight = 0;
    for (const QLayoutItem *item : itemList) {
        if (item != nullptr) {
            if (item->widget()) {
                QSize itemSize = item->widget()->sizeHint();
                maxItemWidth = std::max(maxItemWidth, itemSize.width());
                maxItemHeight = std::max(maxItemHeight, itemSize.height());
            }
        }
    }

    // Calculate the overlap offsets.
    int extra_for_overlap = (100 - overlapPercentage);
    int overlapOffsetWidth =
        (direction == Qt::Horizontal) ? static_cast<int>(std::round((maxItemWidth / 100.0) * extra_for_overlap)) : 0;
    int overlapOffsetHeight =
        (direction == Qt::Vertical) ? static_cast<int>(std::round((maxItemHeight / 100.0) * extra_for_overlap)) : 0;

    // Variables to hold the total dimensions based on layout orientation.
    int totalWidth = 0;
    int totalHeight = 0;

    // Calculate the total size based on the layout direction and constraints.
    if (direction == Qt::Horizontal) {
        // Determine the number of columns:
        // - Use maxColumns if it is greater than 0 (constraint set by the user).
        // - Otherwise, set the number of columns to the total number of items in the layout.
        int numColumns = (maxColumns > 0) ? maxColumns : itemList.size();

        // Calculate the extra space required for overlaps between columns:
        // - Each overlap reduces the effective width of subsequent items.
        int extra_space_for_overlaps = overlapOffsetWidth * (numColumns - 1);

        // Total width:
        // - The first item's width is fully included (maxItemWidth).
        // - Add the space for all overlaps between adjacent items.
        totalWidth = maxItemWidth + extra_space_for_overlaps;

        // Determine the number of rows:
        // - Use maxRows if maxColumns is set (to constrain the number of rows).
        // - Otherwise, assume a single row.
        int numRows = (maxColumns > 0) ? ceil(itemList.size() / static_cast<double>(numColumns)) : 1;

        // Total height:
        // - Multiply the number of rows by the item height (maxItemHeight).
        // - Subtract the height overlap between rows to avoid double-counting.
        totalHeight = maxItemHeight * numRows - (numRows - 1) * overlapOffsetHeight;
    } else if (direction == Qt::Vertical) {
        // Determine the number of columns:
        // - Use maxRows to calculate how many columns are needed if a row constraint exists.
        // - Otherwise, assume a single column.
        int numColumns = (maxRows != 0) ? ceil(itemList.size() / static_cast<double>(maxRows)) : 1;

        // Total width:
        // - Multiply the number of columns by the item width (maxItemWidth).
        // - Subtract the width overlap between columns to avoid double-counting.
        totalWidth = maxItemWidth * numColumns - (numColumns - 1) * overlapOffsetWidth;

        // Determine the number of rows:
        // - Use maxRows if it is greater than 0 (constraint set by the user).
        // - Otherwise, set the number of rows to the total number of items in the layout.
        int numRows = (maxRows > 0) ? maxRows : itemList.size();

        // Calculate the extra space required for overlaps between rows:
        // - Each overlap reduces the effective height of subsequent items.
        int extra_space_for_overlaps = overlapOffsetHeight * (numRows - 1);

        // Total height:
        // - The first item's height is fully included (maxItemHeight).
        // - Add the space for all overlaps between adjacent items.
        totalHeight = maxItemHeight + extra_space_for_overlaps;
    }

    // Assign the calculated dimensions to the preferred size.
    preferredSize = QSize(totalWidth, totalHeight);

    return preferredSize;
}

/**
 * @brief Returns the size hint for the layout, based on preferred size calculations.
 *
 * Provides a recommended size for the layout, useful for layouts that need to fit within
 * a specific parent container size. This takes into account the preferred size and
 * any specific item size requirements.
 *
 * @return The layout's recommended QSize.
 */
QSize OverlapLayout::sizeHint() const
{
    return calculatePreferredSize();
}

/**
 * @brief Provides the minimum size hint for the layout, ensuring functionality within constraints.
 *
 * Defines a minimum workable size for the layout to prevent excessive compression
 * that could distort item arrangement.
 *
 * @return The minimum QSize for this layout.
 */
QSize OverlapLayout::minimumSize() const
{
    return calculatePreferredSize();
}

/**
 * @brief Sets the layout's orientation direction.
 *
 * @param new_direction The new orientation direction (Qt::Horizontal or Qt::Vertical).
 */
void OverlapLayout::setDirection(Qt::Orientation new_direction)
{
    direction = new_direction;
}

/**
 * @brief Sets the maximum number of columns for horizontal orientation.
 *
 * @param newValue New maximum column count.
 */
void OverlapLayout::setMaxColumns(int newValue)
{
    if (newValue >= 0) {
        maxColumns = newValue;
    }
}

/**
 * @brief Sets the maximum number of rows for vertical orientation.
 *
 * @param newValue New maximum row count.
 */
void OverlapLayout::setMaxRows(int newValue)
{
    if (newValue >= 0) {
        maxRows = newValue;
    }
}

/**
 * @brief Calculates the maximum number of columns for a vertical overlap layout based on the current width.
 *
 * This function determines the maximum number of columns that can fit within the layout's width
 * given the overlap percentage and item size, based on the current layout direction.
 *
 * @return Maximum number of columns that can fit within the layout width.
 */
int OverlapLayout::calculateMaxColumns() const
{
    if (direction != Qt::Vertical || itemList.isEmpty()) {
        return 1; // Only relevant if the layout direction is vertical
    }

    // Determine maximum item width
    int maxItemWidth = 0;
    for (const QLayoutItem *item : itemList) {
        if (item->widget()) {
            QSize itemSize = item->widget()->sizeHint();
            maxItemWidth = std::max(maxItemWidth, itemSize.width());
        }
    }

    int availableWidth = parentWidget() ? parentWidget()->width() : 0;
    // Determine the maximum number of columns that can fit
    int columns = availableWidth / maxItemWidth;

    return qMax(1, columns);
}

/**
 * @brief Calculates the maximum number of rows needed for a given number of columns in a vertical overlap layout.
 *
 * Determines how many rows are required to arrange all items given the calculated or specified number of columns.
 *
 * @param columns The number of columns available.
 * @return The total number of rows required.
 */
int OverlapLayout::calculateRowsForColumns(int columns) const
{
    if (direction != Qt::Vertical || itemList.isEmpty() || columns <= 0) {
        return 1; // Only relevant if the layout direction is vertical and there are items
    }

    int totalItems = itemList.size();
    int rows = std::ceil(static_cast<double>(totalItems) / columns);

    return rows;
}

/**
 * @brief Calculates the maximum number of rows for a horizontal overlap layout based on the current height.
 *
 * This function determines the maximum number of rows that can fit within the layout's height
 * given the overlap percentage and item size, based on the current layout direction.
 *
 * @return Maximum number of rows that can fit within the layout height.
 */
int OverlapLayout::calculateMaxRows() const
{
    if (direction != Qt::Horizontal || itemList.isEmpty()) {
        return 1; // Only relevant if the layout direction is horizontal
    }

    // Determine maximum item height
    int maxItemHeight = 0;
    for (const QLayoutItem *item : itemList) {
        if (item->widget()) {
            QSize itemSize = item->widget()->sizeHint();
            maxItemHeight = std::max(maxItemHeight, itemSize.height());
        }
    }

    // Calculate the effective height of each item with the overlap applied
    int overlapOffsetHeight = (maxItemHeight * (100 - overlapPercentage)) / 100;
    int availableHeight = parentWidget() ? parentWidget()->height() : 0;

    // Determine the maximum number of rows that can fit
    int rows = availableHeight / overlapOffsetHeight;

    return qMax(1, rows);
}

/**
 * @brief Calculates the maximum number of columns needed for a given number of rows in a horizontal overlap layout.
 *
 * Determines how many columns are required to arrange all items given the calculated or specified number of rows.
 *
 * @param rows The number of rows available.
 * @return The total number of columns required.
 */
int OverlapLayout::calculateColumnsForRows(int rows) const
{
    if (direction != Qt::Horizontal || itemList.isEmpty() || rows <= 0) {
        return 1; // Only relevant if the layout direction is horizontal and there are items
    }

    int totalItems = itemList.size();
    int columns = std::ceil(static_cast<double>(totalItems) / rows);

    return columns;
}