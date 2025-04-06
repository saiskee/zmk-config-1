FROM zmkfirmware/zmk-build-arm:stable

# Install any additional tools that might be needed
RUN apt-get update && apt-get install -y \
    git \
    wget \
    curl \
    python3 \
    python3-pip \
    && rm -rf /var/lib/apt/lists/*

# Set up non-root user for VS Code
ARG USERNAME=vscode
ARG USER_UID=1000
ARG USER_GID=$USER_UID

RUN groupadd --gid $USER_GID $USERNAME \
    && useradd --uid $USER_UID --gid $USER_GID -m $USERNAME \
    && apt-get update \
    && apt-get install -y sudo \
    && echo $USERNAME ALL=\(root\) NOPASSWD:ALL > /etc/sudoers.d/$USERNAME \
    && chmod 0440 /etc/sudoers.d/$USERNAME

# Add user to dialout group for USB access
RUN usermod -aG dialout $USERNAME

# Set up environment variables
ENV DEBIAN_FRONTEND=noninteractive
ENV ZEPHYR_VERSION=3.5.0
