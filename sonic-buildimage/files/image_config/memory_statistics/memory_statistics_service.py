import os
import json
from sonic_buildimage.logger import logger

class MemoryStatisticsService:
    def write_memory_statistics(self, total_dict, sysmem_dict):
        """
        Writes memory statistics to a JSON file.
        
        Args:
            total_dict (dict): Dictionary containing total memory statistics.
            sysmem_dict (dict): Dictionary containing system memory statistics.
        
        Raises:
            FileNotFoundError: If the output directory does not exist.
            PermissionError: If there are insufficient permissions to write the file.
            json.JSONDecodeError: If there is an error encoding the data to JSON.
            OSError: If there is an error writing the file.
        """
        try:
            output_dir = os.path.dirname(self.config['output_file'])
            if not os.path.exists(output_dir):
                os.makedirs(output_dir)

            with open(self.config['output_file'], 'w') as f:
                json.dump({
                    'total': total_dict,
                    'system': sysmem_dict
                }, f, indent=4)

        except FileNotFoundError as e:
            logger.log_error(f"Output directory not found: {e}")
            raise
        except PermissionError as e:
            logger.log_error(f"Insufficient permissions to write to {self.config['output_file']}: {e}")
            raise
        except json.JSONDecodeError as e:
            logger.log_error(f"Error encoding memory statistics to JSON: {e}")
            raise
        except OSError as e:
            logger.log_error(f"Error writing memory statistics file: {e}")
            raise 