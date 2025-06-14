class LoggerClass:
    def __init__(self, identifier):
        self.identifier = identifier
    
    def log_notice(self, msg):
        print(f"NOTICE [{self.identifier}]: {msg}")
    
    def log_warning(self, msg):
        print(f"WARNING [{self.identifier}]: {msg}")
    
    def log_info(self, msg):
        print(f"INFO [{self.identifier}]: {msg}")
    
    def log_error(self, msg):
        print(f"ERROR [{self.identifier}]: {msg}")

def Logger(identifier):
    return LoggerClass(identifier)