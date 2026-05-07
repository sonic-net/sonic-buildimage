#!/usr/bin/env python3
import sys

# Expect DB names as command-line arguments
db_names = sys.argv[1:]

if not db_names:
    print('ERROR:No database names provided as arguments', file=sys.stderr)
    sys.exit(1)

try:
    import swsscommon.swsscommon

    SonicDBConfig = None

    if hasattr(swsscommon.swsscommon, 'SonicDBConfig'):
        SonicDBConfig = swsscommon.swsscommon.SonicDBConfig

    if SonicDBConfig is None:
        print('ERROR:SonicDBConfig not found in swsscommon module', file=sys.stderr)
        sys.exit(1)

    # Initialize config (some versions require this)
    try:
        SonicDBConfig.loadSonicDBConfig()
    except AttributeError:
        pass  # Some versions don't have this method
    except Exception:
        pass  # Ignore if config file is missing

    # Iterate over provided arguments
    for db in db_names:
        try:
            db_id = SonicDBConfig.getDbId(db)
            db_port = SonicDBConfig.getDbPort(db)
            # Output format: DB_NAME:DB_ID:DB_PORT
            print(f'{db}:{db_id}:{db_port}')
        except Exception as e:
            # Print error to stderr (will be captured by bash)
            print(f'ERROR:Failed to get config for {db}: {e}', file=sys.stderr)

except ImportError as e:
    print(f'ERROR:swsscommon module not found: {e}', file=sys.stderr)
    sys.exit(1)
except Exception as e:
    print(f'ERROR:Unexpected error: {e}', file=sys.stderr)
    sys.exit(1)