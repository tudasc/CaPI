#!/usr/bin/env python3
"""
Auto-update README.md with generated selector tables.

This script runs the SelectorTableGenerator, parses the output, and 
automatically updates the DEFAULT and TALP selector tables in README.md.
"""

import subprocess
import json
import sys
import os

def run_generator():
    """Run the SelectorTableGenerator and return parsed tables."""
    result = subprocess.run(
        ['./build/scripts/SelectorTableGenerator/generator'],
        capture_output=True, text=True
    )
    
    lines = result.stdout.split('\n')
    table_lines = [l for l in lines if l.startswith('|') and '"' in l]
    
    # Parse JSON values from pipe-delimited format
    rows = []
    for line in table_lines:
        parts = []
        current = ""
        in_quotes = False
        in_bracket = False
        for char in line:
            if char == '"' and (not current or current[-1] != '\\'):
                in_quotes = not in_quotes
            elif char == '[':
                in_bracket = True
            elif char == ']':
                in_bracket = False
            elif char == '|' and not in_quotes and not in_bracket:
                parts.append(current.strip())
                current = ""
                continue
            current += char
        if current:
            parts.append(current.strip())
        
        if len(parts) >= 5:
            name = json.loads(parts[1]) if parts[1] else ""
            params_raw = parts[2]
            inputs = json.loads(parts[3]) if parts[3] else ""
            example = json.loads(parts[4]) if parts[4] else ""
            explanation = json.loads(parts[5]) if len(parts) > 5 and parts[5] else ""
            
            try:
                params_list = json.loads(params_raw)
                params = ', '.join(params_list) if params_list else '-'
            except:
                params = params_raw
            
            # Escape pipe characters in examples
            example = example.replace('|', '\\|')
            
            rows.append({
                'name': name,
                'params': params,
                'inputs': inputs,
                'example': example,
                'explanation': explanation
            })
    
    # Separate DEFAULT and TALP
    default_rows = [r for r in rows if not r['name'].startswith('talp_') and r['name'] != 'has_talp_metrics']
    talp_rows = [r for r in rows if r['name'].startswith('talp_') or r['name'] == 'has_talp_metrics']
    
    return default_rows, talp_rows

def format_markdown_table(rows, headers):
    """
    Format rows as markdown table with aligned columns.
    
    Args:
        rows: List of dicts with column data
        headers: List of header names (keys into row dicts)
    
    Returns:
        List of formatted markdown lines
    """
    # Calculate column widths
    col_widths = {header: len(header) for header in headers}
    for row in rows:
        for header in headers:
            val = str(row.get(header, ''))
            col_widths[header] = max(col_widths[header], len(val))
    
    # Format header
    header_cells = [header.ljust(col_widths[header]) for header in headers]
    lines = ["| " + " | ".join(header_cells) + " |"]
    
    # Format separator
    sep_cells = ["-" * col_widths[header] for header in headers]
    lines.append("|" + "|".join(sep_cells) + "|")
    
    # Format data rows
    for row in rows:
        cells = []
        for header in headers:
            val = str(row.get(header, ''))
            cells.append(val.ljust(col_widths[header]))
        lines.append("| " + " | ".join(cells) + " |")
    
    return lines

def format_table_with_prettier(table_lines):
    """
    Format table markdown with prettier.
    
    Args:
        table_lines: List of markdown table lines
    
    Returns:
        List of formatted markdown lines, or original lines if prettier fails
    """
    try:
        import subprocess
        # Join lines and format with prettier via stdin
        table_text = '\n'.join(table_lines)
        result = subprocess.run(
            ['npx', '-y', 'prettier', '--parser', 'markdown', '--prose-wrap', 'preserve'],
            input=table_text,
            capture_output=True,
            text=True,
            timeout=30
        )
        if result.returncode == 0:
            # Remove trailing newline and split back into lines
            formatted = result.stdout.rstrip('\n')
            return formatted.split('\n')
        else:
            return table_lines
    except Exception:
        # prettier not available, return original
        return table_lines

def generate_default_table(rows):
    """Generate markdown table for DEFAULT selectors matching README format."""
    headers = ["Name", "Parameters", "Selector inputs", "Example", "Explanation"]
    # Transform rows for table formatting
    table_rows = []
    for r in rows:
        table_rows.append({
            "Name": r['name'],
            "Parameters": r['params'],
            "Selector inputs": r['inputs'],
            "Example": f"`{r['example']}`",
            "Explanation": r['explanation']
        })
    # Generate table with internal alignment
    table_lines = format_markdown_table(table_rows, headers)
    # Format with prettier
    return format_table_with_prettier(table_lines)

def generate_talp_table(rows):
    """Generate markdown table for TALP selectors."""
    headers = ["Name", "Parameters", "Selector inputs", "Example", "Explanation"]
    # Transform rows for table formatting
    table_rows = []
    for r in rows:
        table_rows.append({
            "Name": r['name'],
            "Parameters": r['params'],
            "Selector inputs": r['inputs'],
            "Example": f"`{r['example']}`",
            "Explanation": r['explanation']
        })
    # Generate table with internal alignment
    table_lines = format_markdown_table(table_rows, headers)
    # Format with prettier
    return format_table_with_prettier(table_lines)

def update_readme(default_rows, talp_rows):
    """Update README.md with generated tables."""
    with open('README.md', 'r') as f:
        lines = f.readlines()
    
    # Find and replace DEFAULT table
    list_idx = None
    for i, line in enumerate(lines):
        if '### List of available selectors' in line:
            list_idx = i
            break
    
    if list_idx is None:
        print("ERROR: Could not find '### List of available selectors' section")
        return False
    
    # Find table start (first | after the section header)
    table_start = None
    for i in range(list_idx, len(lines)):
        if lines[i].startswith('|'):
            table_start = i
            break
    
    if table_start is None:
        print("ERROR: Could not find start of DEFAULT table")
        return False
    
    # Find table end using HTML comment marker
    table_end = None
    for i in range(table_start, len(lines)):
        if 'CAPI_DEFAULT_SELECTORS_END' in lines[i]:
            table_end = i - 1
            # Skip back over empty lines
            while table_end > table_start and lines[table_end].strip() == '':
                table_end -= 1
            break
    
    if table_end is None:
        print("ERROR: Could not find 'CAPI_DEFAULT_SELECTORS_END' marker")
        return False
    
    # Generate new DEFAULT table
    default_table_lines = generate_default_table(default_rows)
    
    # Replace DEFAULT table
    new_lines = lines[:table_start] + [l + '\n' for l in default_table_lines] + ['\n'] + lines[table_end+1:]
    lines = new_lines
    
    # Find and replace TALP table
    talp_start = None
    for i, line in enumerate(lines):
        if '#### TALP selectors' in line:
            talp_start = i
            break
    
    if talp_start is None:
        print("ERROR: Could not find 'TALP selectors' section")
        return False
    
    # Find TALP table start using HTML comment marker
    talp_table_start = None
    for i in range(talp_start, len(lines)):
        if 'CAPI_TALP_SELECTORS_START' in lines[i]:
            # Next pipe-delimited line after this marker is the table start
            for j in range(i + 1, len(lines)):
                if lines[j].startswith('|'):
                    talp_table_start = j
                    break
            break
    
    if talp_table_start is None:
        print("ERROR: Could not find 'CAPI_TALP_SELECTORS_START' marker")
        return False
    
    # Find TALP table end using HTML comment marker
    talp_table_end = None
    for i in range(talp_table_start, len(lines)):
        if 'CAPI_TALP_SELECTORS_END' in lines[i]:
            talp_table_end = i - 1
            # Skip back over empty lines
            while talp_table_end > talp_table_start and lines[talp_table_end].strip() == '':
                talp_table_end -= 1
            break
    
    if talp_table_end is None:
        print("ERROR: Could not find 'CAPI_TALP_SELECTORS_END' marker")
        return False
    while talp_table_end > talp_table_start and lines[talp_table_end].strip() == '':
        talp_table_end -= 1
    
    # Generate new TALP table
    talp_table_lines = generate_talp_table(talp_rows)
    
    # Replace TALP table
    new_lines = lines[:talp_table_start] + [l + '\n' for l in talp_table_lines] + ['\n'] + lines[talp_table_end+1:]
    
    # Write updated README
    with open('README.md', 'w') as f:
        f.writelines(new_lines)
    
    return True


if __name__ == '__main__':
    # Change to repo root if needed
    if not os.path.exists('build/scripts/SelectorTableGenerator/generator'):
        print("ERROR: SelectorTableGenerator not found. Make sure you're in the CaPI root directory.")
        sys.exit(1)
    
    print("Generating selector tables...")
    default_rows, talp_rows = run_generator()
    print(f"  - DEFAULT selectors: {len(default_rows)}")
    print(f"  - TALP selectors: {len(talp_rows)}")
    
    print("Updating README.md...")
    if update_readme(default_rows, talp_rows):
        print("README.md updated successfully!")
        sys.exit(0)
    else:
        print("Failed to update README.md")
        sys.exit(1)
