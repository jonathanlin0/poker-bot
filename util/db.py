import sqlite3

class DB:
    @staticmethod
    def update_experiment_column(db_path: str, experiment_name: str, column_name: str, new_value: str) -> None:
        """
        Update a specific column in the experiments table for a given experiment name.

        Args:
            db_path (str): The path to the SQLite database file.
            experiment_name (str): The name of the experiment to update.
            column_name (str): The column name to update.
            new_value (str): The new value to set for the specified column.
        """
        # Connect to the SQLite database
        connection = sqlite3.connect(db_path)
        cursor = connection.cursor()

        # Prepare the SQL query to update the specific column
        query = f"UPDATE experiments SET {column_name} = ? WHERE name = ?"

        try:
            # Execute the SQL query
            cursor.execute(query, (new_value, experiment_name))
            # Commit the changes
            connection.commit()
            print(f"Updated {column_name} for experiment '{experiment_name}' successfully.")
        except sqlite3.Error as e:
            print(f"An error occurred: {e}")
        finally:
            # Close the connection
            connection.close()