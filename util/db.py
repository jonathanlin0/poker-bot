import sqlite3

class DB:
    """
    A utility class that lets a user interact with an SQLite database. Contains some helper functions.
    """

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

    @staticmethod
    def get_row_as_dict(db_path, table_name, input_name):
        """
        Fetches a row from the specified SQLite database table based on the input name and returns it as a dictionary.
        
        Args:
            db_path (str): The path to the SQLite database file.
            table_name (str): The name of the table from which to fetch the row.
            input_name (str): The value to search for in the 'name' column.
        
        Returns:
            dict: A dictionary where keys are column names and values are the corresponding row values.
        """
        # Connect to the SQLite database
        connection = sqlite3.connect(db_path)
        cursor = connection.cursor()

        # Prepare the SQL query to fetch the row with the matching name
        query = f"SELECT * FROM {table_name} WHERE name = ?"
        
        try:
            # Execute the SQL query
            cursor.execute(query, (input_name,))
            
            # Fetch the column names
            columns = [column[0] for column in cursor.description]
            
            # Fetch the row with the matching name
            row = cursor.fetchone()

            # If the row is found, create a dictionary with column names as keys and row values as values
            if row:
                result_dict = dict(zip(columns, row))
            else:
                result_dict = None  # Return None if no matching row is found

        except sqlite3.Error as e:
            print(f"An error occurred: {e}")
            result_dict = None
        finally:
            # Close the database connection
            connection.close()

        return result_dict
    