use thiserror::Error;

#[derive(Debug, Error)]
pub enum AppError {
    #[error("{0}")]
    Usage(String),
    #[error("{0}")]
    Operational(String),
}

impl AppError {
    pub fn usage(message: impl Into<String>) -> Self {
        Self::Usage(message.into())
    }

    pub fn operational(message: impl Into<String>) -> Self {
        Self::Operational(message.into())
    }

    pub fn exit_code(&self) -> u8 {
        match self {
            Self::Usage(_) => 2,
            Self::Operational(_) => 1,
        }
    }
}

pub type Result<T> = std::result::Result<T, AppError>;
