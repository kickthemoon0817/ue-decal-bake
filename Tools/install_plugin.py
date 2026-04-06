#!/usr/bin/env python3
"""
DecalBaker Plugin Installer
============================

Simple drag-and-drop installer for designers.
Run this script and it will find your UE project and install the plugin.

Usage:
    python install_plugin.py
    python install_plugin.py /path/to/MyProject.uproject
    python install_plugin.py --uninstall /path/to/MyProject.uproject
"""

import os
import sys
import shutil
import json
import glob


def find_plugin_source():
    """Find the DecalBaker plugin folder relative to this script."""
    script_dir = os.path.dirname(os.path.abspath(__file__))

    # Check sibling folder (when distributed in zip)
    candidate = os.path.join(script_dir, "DecalBaker")
    if os.path.isdir(candidate) and os.path.exists(os.path.join(candidate, "DecalBaker.uplugin")):
        return candidate

    # Check parent repo structure
    candidate = os.path.join(script_dir, "..", "DecalBaker")
    if os.path.isdir(candidate) and os.path.exists(os.path.join(candidate, "DecalBaker.uplugin")):
        return os.path.abspath(candidate)

    return None


def find_uproject_files():
    """Search common locations for .uproject files."""
    candidates = []

    # Current directory
    for f in glob.glob("*.uproject"):
        candidates.append(os.path.abspath(f))

    # Parent directory
    for f in glob.glob("../*.uproject"):
        candidates.append(os.path.abspath(f))

    # Home directory UE projects
    home = os.path.expanduser("~")
    for search_dir in [
        os.path.join(home, "Documents", "Unreal Projects"),
        os.path.join(home, "UE Projects"),
        os.path.join(home, "UnrealProjects"),
    ]:
        if os.path.isdir(search_dir):
            for root, dirs, files in os.walk(search_dir):
                for f in files:
                    if f.endswith(".uproject"):
                        candidates.append(os.path.join(root, f))
                # Don't recurse too deep
                if root.count(os.sep) - search_dir.count(os.sep) > 2:
                    dirs.clear()

    return list(set(candidates))


def enable_plugin_in_uproject(uproject_path):
    """Add DecalBaker to the plugin list in the .uproject file."""
    with open(uproject_path, "r") as f:
        data = json.load(f)

    plugins = data.get("Plugins", [])

    # Check if already enabled
    for plugin in plugins:
        if plugin.get("Name") == "DecalBaker":
            plugin["Enabled"] = True
            print("  Plugin entry already exists, ensuring it's enabled")
            with open(uproject_path, "w") as f:
                json.dump(data, f, indent="\t")
            return

    # Add new entry
    plugins.append({
        "Name": "DecalBaker",
        "Enabled": True
    })
    data["Plugins"] = plugins

    with open(uproject_path, "w") as f:
        json.dump(data, f, indent="\t")

    print("  Added DecalBaker to plugin list")


def disable_plugin_in_uproject(uproject_path):
    """Remove DecalBaker from the plugin list in the .uproject file."""
    with open(uproject_path, "r") as f:
        data = json.load(f)

    plugins = data.get("Plugins", [])
    data["Plugins"] = [p for p in plugins if p.get("Name") != "DecalBaker"]

    with open(uproject_path, "w") as f:
        json.dump(data, f, indent="\t")

    print("  Removed DecalBaker from plugin list")


def install(uproject_path):
    """Install DecalBaker into the target project."""
    project_dir = os.path.dirname(uproject_path)
    project_name = os.path.basename(uproject_path)
    plugins_dir = os.path.join(project_dir, "Plugins")
    target_dir = os.path.join(plugins_dir, "DecalBaker")

    plugin_source = find_plugin_source()
    if not plugin_source:
        print("ERROR: Cannot find DecalBaker plugin folder.")
        print("Make sure this script is in the same folder as the DecalBaker/ directory.")
        return False

    print()
    print("=" * 50)
    print("  DecalBaker Plugin Installer")
    print("=" * 50)
    print(f"  Project:  {project_name}")
    print(f"  Location: {project_dir}")
    print(f"  Source:   {plugin_source}")
    print("=" * 50)
    print()

    # Check if already installed
    if os.path.isdir(target_dir):
        print("  DecalBaker is already installed.")
        answer = input("  Reinstall / update? [y/N]: ").strip().lower()
        if answer != "y":
            print("  Cancelled.")
            return False
        shutil.rmtree(target_dir)
        print("  Removed old installation.")

    # Copy plugin
    print("  Copying plugin files...")
    os.makedirs(plugins_dir, exist_ok=True)
    shutil.copytree(plugin_source, target_dir)
    print("  Plugin files copied.")

    # Enable in .uproject
    print("  Updating project file...")
    enable_plugin_in_uproject(uproject_path)

    print()
    print("=" * 50)
    print("  Installation complete!")
    print("=" * 50)
    print()
    print("  Next steps:")
    print("  1. Open your project in Unreal Engine")
    print("  2. If prompted to rebuild, click Yes")
    print("  3. Find 'Decal Baker' in the toolbar")
    print()
    return True


def uninstall(uproject_path):
    """Remove DecalBaker from the target project."""
    project_dir = os.path.dirname(uproject_path)
    target_dir = os.path.join(project_dir, "Plugins", "DecalBaker")

    print()
    print("  Uninstalling DecalBaker...")

    if os.path.isdir(target_dir):
        shutil.rmtree(target_dir)
        print("  Plugin files removed.")
    else:
        print("  Plugin files not found (already removed).")

    disable_plugin_in_uproject(uproject_path)

    print("  Uninstall complete.")
    print()
    return True


def select_project():
    """Interactive project selection."""
    print()
    print("  Where do you want to install DecalBaker?")
    print()

    # Find projects
    projects = find_uproject_files()

    if projects:
        print("  Found UE projects:")
        for i, p in enumerate(projects, 1):
            name = os.path.basename(p)
            location = os.path.dirname(p)
            print(f"    {i}. {name}  ({location})")
        print(f"    {len(projects) + 1}. Enter path manually")
        print()

        choice = input(f"  Select [1-{len(projects) + 1}]: ").strip()
        try:
            idx = int(choice)
            if 1 <= idx <= len(projects):
                return projects[idx - 1]
        except ValueError:
            pass

    # Manual entry
    print()
    path = input("  Enter full path to your .uproject file: ").strip()
    path = path.strip('"').strip("'")

    if os.path.isdir(path):
        # They gave a directory, look for .uproject inside
        uprojects = glob.glob(os.path.join(path, "*.uproject"))
        if uprojects:
            return uprojects[0]

    if path.endswith(".uproject") and os.path.exists(path):
        return path

    print(f"  ERROR: Cannot find .uproject file at '{path}'")
    return None


def main():
    is_uninstall = "--uninstall" in sys.argv

    # Get project path from args or interactive selection
    uproject_path = None
    for arg in sys.argv[1:]:
        if arg.endswith(".uproject") and os.path.exists(arg):
            uproject_path = os.path.abspath(arg)
        elif os.path.isdir(arg):
            uprojects = glob.glob(os.path.join(arg, "*.uproject"))
            if uprojects:
                uproject_path = os.path.abspath(uprojects[0])

    if not uproject_path:
        uproject_path = select_project()

    if not uproject_path:
        print("  No project selected. Exiting.")
        sys.exit(1)

    if is_uninstall:
        success = uninstall(uproject_path)
    else:
        success = install(uproject_path)

    sys.exit(0 if success else 1)


if __name__ == "__main__":
    main()
